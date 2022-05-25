// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/PreIndexedPackageSourceFactory.h"
#include "Microsoft/SQLiteIndex.h"
#include "Microsoft/SQLiteIndexSource.h"

#include <AppInstallerDeployment.h>
#include <AppInstallerMsixInfo.h>
#include <winget/ManagedFile.h>

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace AppInstaller::Repository::Microsoft
{
    namespace
    {
        static constexpr std::string_view s_PreIndexedPackageSourceFactory_PackageFileName = "source.msix"sv;
        static constexpr std::string_view s_PreIndexedPackageSourceFactory_IndexFileName = "index.db"sv;
        // TODO: This being hard coded to force using the Public directory name is not ideal.
        static constexpr std::string_view s_PreIndexedPackageSourceFactory_IndexFilePath = "Public\\index.db"sv;

        // Construct the package location from the given details.
        // Currently expects that the arg is an https uri pointing to the root of the data.
        std::string GetPackageLocation(const SourceDetails& details)
        {
            THROW_HR_IF(E_INVALIDARG, details.Arg.empty());
            std::string result = details.Arg;
            if (result.back() != '/')
            {
                result += '/';
            }
            result += s_PreIndexedPackageSourceFactory_PackageFileName;
            return result;
        }

        // Gets the package family name from the details.
        std::string GetPackageFamilyNameFromDetails(const SourceDetails& details)
        {
            THROW_HR_IF(E_UNEXPECTED, details.Data.empty());
            return details.Data;
        }

        // Creates a name for the cross process reader-writer lock given the details.
        std::string CreateNameForCPRWL(const SourceDetails& details)
        {
            // The only relevant data is the package family name
            return "PreIndexedSourceCPRWL_"s + GetPackageFamilyNameFromDetails(details);
        }

        // The base class for a package that comes from a preindexed packaged source.
        struct PreIndexedFactoryBase : public ISourceFactory
        {
            std::shared_ptr<ISourceReference> Create(const SourceDetails& details) override final
            {
                // With more than one source implementation, we will probably need to probe first
                THROW_HR_IF(E_INVALIDARG, !details.Type.empty() && details.Type != PreIndexedPackageSourceFactory::Type());

                return CreateInternal(details);
            }

            virtual std::shared_ptr<ISourceReference> CreateInternal(const SourceDetails& details) = 0;

            bool Add(SourceDetails& details, IProgressCallback& progress) override final
            {
                if (details.Type.empty())
                {
                    // With more than one source implementation, we will probably need to probe first
                    details.Type = PreIndexedPackageSourceFactory::Type();
                    AICLI_LOG(Repo, Info, << "Initializing source type: " << details.Name << " => " << details.Type);
                }
                else
                {
                    THROW_HR_IF(E_INVALIDARG, details.Type != PreIndexedPackageSourceFactory::Type());
                }

                std::string packageLocation = GetPackageLocation(details);

                THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_NOT_SECURE, Utility::IsUrlRemote(packageLocation) && !Utility::IsUrlSecure(packageLocation));

                AICLI_LOG(Repo, Info, << "Initializing source from: " << details.Name << " => " << packageLocation);

                Msix::MsixInfo packageInfo(packageLocation);
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_PACKAGE_IS_BUNDLE, packageInfo.GetIsBundle());

                auto fullName = packageInfo.GetPackageFullName();
                AICLI_LOG(Repo, Info, << "Found package full name: " << details.Name << " => " << fullName);

                details.Data = Msix::GetPackageFamilyNameFromFullName(fullName);
                details.Identifier = Msix::GetPackageFamilyNameFromFullName(fullName);

                auto lock = LockExclusive(details, progress);
                if (!lock)
                {
                    return false;
                }

                return UpdateInternal(packageLocation, packageInfo, details, progress);
            }

            bool Update(const SourceDetails& details, IProgressCallback& progress) override final
            {
                return UpdateBase(details, false, progress);
            }

            bool BackgroundUpdate(const SourceDetails& details, IProgressCallback& progress) override final
            {
                return UpdateBase(details, true, progress);
            }

            virtual bool UpdateInternal(const std::string& packageLocation, Msix::MsixInfo& packageInfo, const SourceDetails& details, IProgressCallback& progress) = 0;

            bool Remove(const SourceDetails& details, IProgressCallback& progress) override final
            {
                THROW_HR_IF(E_INVALIDARG, details.Type != PreIndexedPackageSourceFactory::Type());
                auto lock = LockExclusive(details, progress);
                if (!lock)
                {
                    return false;
                }

                return RemoveInternal(details, progress);
            }

            virtual bool RemoveInternal(const SourceDetails& details, IProgressCallback&) = 0;

        private:
            Synchronization::CrossProcessReaderWriteLock LockExclusive(const SourceDetails& details, IProgressCallback& progress, bool isBackground = false)
            {
                if (isBackground)
                {
                    // If this is a background update, don't wait on the lock.
                    return Synchronization::CrossProcessReaderWriteLock::LockExclusive(CreateNameForCPRWL(details), 0ms);
                }
                else
                {
                    return Synchronization::CrossProcessReaderWriteLock::LockExclusive(CreateNameForCPRWL(details), progress);
                }
            }

            bool UpdateBase(const SourceDetails& details, bool isBackground, IProgressCallback& progress)
            {
                THROW_HR_IF(E_INVALIDARG, details.Type != PreIndexedPackageSourceFactory::Type());

                std::string packageLocation = GetPackageLocation(details);
                Msix::MsixInfo packageInfo(packageLocation);

                // The package should not be a bundle
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_PACKAGE_IS_BUNDLE, packageInfo.GetIsBundle());

                // Ensure that family name has not changed
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE,
                    GetPackageFamilyNameFromDetails(details) != Msix::GetPackageFamilyNameFromFullName(packageInfo.GetPackageFullName()));

                if (progress.IsCancelled())
                {
                    AICLI_LOG(Repo, Info, << "Cancelling update upon request");
                    return false;
                }

                auto lock = LockExclusive(details, progress, isBackground);
                if (!lock)
                {
                    return false;
                }

                return UpdateInternal(packageLocation, packageInfo, details, progress);
            }
        };

        // *Should only be called when under a CrossProcessReaderWriteLock*
        std::optional<Deployment::Extension> GetExtensionFromDetails(const SourceDetails& details)
        {
            Deployment::ExtensionCatalog catalog(Deployment::SourceExtensionName);
            return catalog.FindByPackageFamilyAndId(GetPackageFamilyNameFromDetails(details), Deployment::IndexDBId);
        }

        struct PackagedContextSourceReference : public ISourceReference
        {
            PackagedContextSourceReference(const SourceDetails& details) : m_details(details)
            {
                if (!m_details.Data.empty())
                {
                    m_details.Identifier = GetPackageFamilyNameFromDetails(details);
                }
            }

            std::string GetIdentifier() override { return m_details.Identifier; }

            SourceDetails& GetDetails() override { return m_details; };

            std::shared_ptr<ISource> Open(IProgressCallback& progress) override
            {
                auto lock = Synchronization::CrossProcessReaderWriteLock::LockShared(CreateNameForCPRWL(m_details), progress);
                if (!lock)
                {
                    return {};
                }

                auto extension = GetExtensionFromDetails(m_details);
                if (!extension)
                {
                    AICLI_LOG(Repo, Info, << "Package not found " << m_details.Data);
                    THROW_HR(APPINSTALLER_CLI_ERROR_SOURCE_DATA_MISSING);
                }

                THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_NEEDS_REMEDIATION), !extension->VerifyContentIntegrity(progress));

                // To work around an issue with accessing the public folder, we are temporarily
                // constructing the location ourself.  This was already the case for the non-packaged
                // runtime, and we can fix both in the future.  The only problem with this is that
                // the directory in the extension *must* be Public, rather than one set by the creator.
                std::filesystem::path indexLocation = extension->GetPackagePath();
                indexLocation /= s_PreIndexedPackageSourceFactory_IndexFilePath;

                SQLiteIndex index = SQLiteIndex::Open(indexLocation.u8string(), SQLiteIndex::OpenDisposition::Immutable);

                // We didn't use to store the source identifier, so we compute it here in case it's
                // missing from the details.
                m_details.Identifier = GetPackageFamilyNameFromDetails(m_details);
                return std::make_shared<SQLiteIndexSource>(m_details, std::move(index), std::move(lock));
            }

        private:
            SourceDetails m_details;
        };

        // Source factory for running within a packaged context
        struct PackagedContextFactory : public PreIndexedFactoryBase
        {
            std::shared_ptr<ISourceReference> CreateInternal(const SourceDetails& details) override
            {
                return std::make_shared<PackagedContextSourceReference>(details);
            }

            bool UpdateInternal(const std::string& packageLocation, Msix::MsixInfo& packageInfo, const SourceDetails& details, IProgressCallback& progress) override
            {
                // Check if the package is newer before calling into deployment.
                // This can save us a lot of time over letting deployment detect same version.
                auto extension = GetExtensionFromDetails(details);
                if (extension)
                {
                    if (!packageInfo.IsNewerThan(extension->GetPackageVersion()))
                    {
                        AICLI_LOG(Repo, Info, << "Remote source data was not newer than existing, no update needed");
                        return true;
                    }
                }

                if (progress.IsCancelled())
                {
                    AICLI_LOG(Repo, Info, << "Cancelling update upon request");
                    return false;
                }

                // Due to complications with deployment, download the file and deploy from
                // a local source while we investigate further.
                bool download = Utility::IsUrlRemote(packageLocation);
                std::filesystem::path tempFile;
                winrt::Windows::Foundation::Uri uri = nullptr;

                if (download)
                {
                    tempFile = Runtime::GetPathTo(Runtime::PathName::Temp);
                    tempFile /= GetPackageFamilyNameFromDetails(details) + ".msix";

                    Utility::Download(packageLocation, tempFile, Utility::DownloadType::Index, progress);

                    uri = winrt::Windows::Foundation::Uri(tempFile.c_str());
                }
                else
                {
                    uri = winrt::Windows::Foundation::Uri(Utility::ConvertToUTF16(packageLocation));
                }

                Deployment::AddPackage(
                    uri,
                    winrt::Windows::Management::Deployment::DeploymentOptions::None,
                    WI_IsFlagSet(details.TrustLevel, SourceTrustLevel::Trusted),
                    progress);

                if (download)
                {
                    try
                    {
                        // If successful, delete the file
                        std::filesystem::remove(tempFile);
                    }
                    CATCH_LOG();
                }

                // Ensure origin if necessary
                // TODO: Move to checking this before deploying it. That requires significant code to be written though
                //       as there is no public API to check the origin directly.
                if (WI_IsFlagSet(details.TrustLevel, SourceTrustLevel::StoreOrigin))
                {
                    std::wstring pfn = packageInfo.GetPackageFullNameWide();

                    PackageOrigin origin = PackageOrigin::PackageOrigin_Unknown;
                    if (SUCCEEDED_WIN32_LOG(GetStagedPackageOrigin(pfn.c_str(), &origin)))
                    {
                        if (origin != PackageOrigin::PackageOrigin_Store)
                        {
                            Deployment::RemovePackage(Utility::ConvertToUTF8(pfn), progress);
                            THROW_HR(APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE);
                        }
                    }
                }

                return true;
            }

            bool RemoveInternal(const SourceDetails& details, IProgressCallback& callback) override
            {
                auto fullName = Msix::GetPackageFullNameFromFamilyName(GetPackageFamilyNameFromDetails(details));

                if (!fullName)
                {
                    AICLI_LOG(Repo, Info, << "No full name found for family name: " << GetPackageFamilyNameFromDetails(details));
                }
                else
                {
                    AICLI_LOG(Repo, Info, << "Removing package: " << *fullName);
                    Deployment::RemovePackage(*fullName, callback);
                }

                return true;
            }
        };

        // Constructs the location that we will write files to.
        std::filesystem::path GetStatePathFromDetails(const SourceDetails& details)
        {
            std::filesystem::path result = Runtime::GetPathTo(Runtime::PathName::LocalState);
            result /= PreIndexedPackageSourceFactory::Type();
            result /= GetPackageFamilyNameFromDetails(details);
            return result;
        }

        struct DesktopContextSourceReference : public ISourceReference
        {
            DesktopContextSourceReference(const SourceDetails& details) : m_details(details)
            {
                if (!m_details.Data.empty())
                {
                    m_details.Identifier = GetPackageFamilyNameFromDetails(details);
                }
            }

            std::string GetIdentifier() override { return m_details.Identifier; }

            SourceDetails& GetDetails() override { return m_details; };

            std::shared_ptr<ISource> Open(IProgressCallback& progress) override
            {
                auto lock = Synchronization::CrossProcessReaderWriteLock::LockShared(CreateNameForCPRWL(m_details), progress);
                if (!lock)
                {
                    return {};
                }

                std::filesystem::path packageLocation = GetStatePathFromDetails(m_details);
                packageLocation /= s_PreIndexedPackageSourceFactory_PackageFileName;

                if (!std::filesystem::exists(packageLocation))
                {
                    AICLI_LOG(Repo, Info, << "Data not found at " << packageLocation);
                    THROW_HR(APPINSTALLER_CLI_ERROR_SOURCE_DATA_MISSING);
                }

                // Put a write exclusive lock on the index package.
                Msix::WriteLockedMsixFile indexPackage{ packageLocation };

                // Validate index package trust info.
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE, !indexPackage.ValidateTrustInfo(WI_IsFlagSet(m_details.TrustLevel, SourceTrustLevel::StoreOrigin)));

                // Create a temp lock exclusive index file.
                auto tempIndexFilePath = Runtime::GetNewTempFilePath();
                auto tempIndexFile = Utility::ManagedFile::CreateWriteLockedFile(tempIndexFilePath, GENERIC_WRITE, true);

                // Populate temp index file.
                Msix::MsixInfo packageInfo(packageLocation);
                packageInfo.WriteToFileHandle(s_PreIndexedPackageSourceFactory_IndexFilePath, tempIndexFile.GetFileHandle(), progress);

                if (progress.IsCancelled())
                {
                    AICLI_LOG(Repo, Info, << "Cancelling open upon request");
                    return {};
                }

                SQLiteIndex index = SQLiteIndex::Open(tempIndexFile.GetFilePath().u8string(), SQLiteIndex::OpenDisposition::Immutable, std::move(tempIndexFile));

                // We didn't use to store the source identifier, so we compute it here in case it's
                // missing from the details.
                m_details.Identifier = GetPackageFamilyNameFromDetails(m_details);
                return std::make_shared<SQLiteIndexSource>(m_details, std::move(index), std::move(lock));
            }

        private:
            SourceDetails m_details;
        };

        // Source factory for running outside of a package.
        struct DesktopContextFactory : public PreIndexedFactoryBase
        {
            std::shared_ptr<ISourceReference> CreateInternal(const SourceDetails& details) override
            {
                return std::make_shared<DesktopContextSourceReference>(details);
            }

            bool UpdateInternal(const std::string& packageLocation, Msix::MsixInfo& packageInfo, const SourceDetails& details, IProgressCallback& progress) override
            {
                // We will extract the manifest and index files directly to this location
                std::filesystem::path packageState = GetStatePathFromDetails(details);
                std::filesystem::create_directories(packageState);

                std::filesystem::path packagePath = packageState / s_PreIndexedPackageSourceFactory_PackageFileName;

                if (std::filesystem::exists(packagePath))
                {
                    // If we already have a trusted index package, use it to determine if we need to update or not.
                    Msix::WriteLockedMsixFile indexPackage{ packagePath };
                    if (indexPackage.ValidateTrustInfo(WI_IsFlagSet(details.TrustLevel, SourceTrustLevel::StoreOrigin)) &&
                        !packageInfo.IsNewerThan(packagePath))
                    {
                        AICLI_LOG(Repo, Info, << "Remote source data was not newer than existing, no update needed");
                        return true;
                    }
                }

                std::filesystem::path tempPackagePath = packagePath.u8string() + ".dnld.msix";
                if (Utility::IsUrlRemote(packageLocation))
                {
                    AppInstaller::Utility::Download(packageLocation, tempPackagePath, AppInstaller::Utility::DownloadType::Index, progress);
                }
                else
                {
                    std::filesystem::copy(packageLocation, tempPackagePath);
                    progress.OnProgress(100, 100, ProgressType::Percent);
                }

                bool updateSuccess = false;
                if (progress.IsCancelled())
                {
                    AICLI_LOG(Repo, Info, << "Cancelling update upon request");
                }
                else
                {
                    bool tempIndexPackageTrusted = false;

                    {
                        // Extra scope to release the file lock right after trust validation.
                        Msix::WriteLockedMsixFile tempIndexPackage{ tempPackagePath };
                        tempIndexPackageTrusted = tempIndexPackage.ValidateTrustInfo(WI_IsFlagSet(details.TrustLevel, SourceTrustLevel::StoreOrigin));
                    }

                    if (tempIndexPackageTrusted)
                    {
                        std::filesystem::rename(tempPackagePath, packagePath);
                        AICLI_LOG(Repo, Info, << "Source update success.");
                        updateSuccess = true;
                    }
                    else
                    {
                        AICLI_LOG(Repo, Error, << "Source update failed. Source package failed trust validation.");
                    }
                }

                if (!updateSuccess)
                {
                    try
                    {
                        std::filesystem::remove(tempPackagePath);
                    }
                    catch (...)
                    {
                        AICLI_LOG(Repo, Info, << "Failed to remove temp index file at: " << tempPackagePath);
                    }
                }

                return updateSuccess;
            }

            bool RemoveInternal(const SourceDetails& details, IProgressCallback&) override
            {
                std::filesystem::path packageState = GetStatePathFromDetails(details);

                if (!std::filesystem::exists(packageState))
                {
                    AICLI_LOG(Repo, Info, << "No state found for source: " << packageState.u8string());
                }
                else
                {
                    AICLI_LOG(Repo, Info, << "Removing state found for source: " << packageState.u8string());
                    std::filesystem::remove_all(packageState);
                }

                return true;
            }
        };
    }

    std::unique_ptr<ISourceFactory> PreIndexedPackageSourceFactory::Create()
    {
        if (Runtime::IsRunningInPackagedContext())
        {
            return std::make_unique<PackagedContextFactory>();
        }
        else
        {
            return std::make_unique<DesktopContextFactory>();
        }
    }
}
