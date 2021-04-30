// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Microsoft/PreIndexedPackageSourceFactory.h"
#include "Microsoft/SQLiteIndex.h"
#include "Microsoft/SQLiteIndexSource.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace AppInstaller::Repository::Microsoft
{
    namespace
    {
        static constexpr std::string_view s_PreIndexedPackageSourceFactory_PackageFileName = "source.msix"sv;
        static constexpr std::string_view s_PreIndexedPackageSourceFactory_AppxManifestFileName = "AppxManifest.xml"sv;
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
            std::shared_ptr<ISource> Create(const SourceDetails& details, IProgressCallback& progress) override final
            {
                THROW_HR_IF(E_INVALIDARG, details.Type != PreIndexedPackageSourceFactory::Type());

                auto lock = Synchronization::CrossProcessReaderWriteLock::LockForRead(CreateNameForCPRWL(details));
                return CreateInternal(details, std::move(lock), progress);
            }

            virtual std::shared_ptr<ISource> CreateInternal(const SourceDetails& details, Synchronization::CrossProcessReaderWriteLock&& lock, IProgressCallback& progress) = 0;

            void Add(SourceDetails& details, IProgressCallback& progress) override final
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

                auto lock = Synchronization::CrossProcessReaderWriteLock::LockForWrite(CreateNameForCPRWL(details));

                UpdateInternal(packageLocation, details, progress);
            }

            void Update(const SourceDetails& details, IProgressCallback& progress) override final
            {
                THROW_HR_IF(E_INVALIDARG, details.Type != PreIndexedPackageSourceFactory::Type());

                std::string packageLocation = GetPackageLocation(details);

                auto lock = Synchronization::CrossProcessReaderWriteLock::LockForWrite(CreateNameForCPRWL(details));

                UpdateInternal(packageLocation, details, progress);
            }

            virtual void UpdateInternal(std::string packageLocation, const SourceDetails& details, IProgressCallback& progress) = 0;

            void Remove(const SourceDetails& details, IProgressCallback& progress) override final
            {
                THROW_HR_IF(E_INVALIDARG, details.Type != PreIndexedPackageSourceFactory::Type());
                auto lock = Synchronization::CrossProcessReaderWriteLock::LockForWrite(CreateNameForCPRWL(details));

                RemoveInternal(details, progress);
            }

            virtual void RemoveInternal(const SourceDetails& details, IProgressCallback&) = 0;
        };

        // Source factory for running within a packaged context
        struct PackagedContextFactory : public PreIndexedFactoryBase
        {
            // *Should only be called when under a CrossProcessReaderWriteLock*
            std::optional<Deployment::Extension> GetExtensionFromDetails(const SourceDetails& details)
            {
                Deployment::ExtensionCatalog catalog(Deployment::SourceExtensionName);
                return catalog.FindByPackageFamilyAndId(GetPackageFamilyNameFromDetails(details), Deployment::IndexDBId);
            }

            std::shared_ptr<ISource> CreateInternal(const SourceDetails& details, Synchronization::CrossProcessReaderWriteLock&& lock, IProgressCallback& progress) override
            {
                auto extension = GetExtensionFromDetails(details);
                if (!extension)
                {
                    AICLI_LOG(Repo, Info, << "Package not found " << details.Data);
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
                return std::make_shared<SQLiteIndexSource>(details, GetPackageFamilyNameFromDetails(details), std::move(index), std::move(lock));
            }

            void UpdateInternal(std::string packageLocation, const SourceDetails& details, IProgressCallback& progress) override
            {
                // Check if the package is newer before calling into deployment.
                // This can save us a lot of time over letting deployment detect same version.
                auto extension = GetExtensionFromDetails(details);
                if (extension)
                {
                    Msix::MsixInfo packageInfo(packageLocation);
                    THROW_HR_IF(APPINSTALLER_CLI_ERROR_PACKAGE_IS_BUNDLE, packageInfo.GetIsBundle());

                    if (progress.IsCancelled())
                    {
                        AICLI_LOG(Repo, Info, << "Cancelling update upon request");
                        return;
                    }

                    if (!packageInfo.IsNewerThan(extension->GetPackageVersion()))
                    {
                        AICLI_LOG(Repo, Info, << "Remote source data was not newer than existing, no update needed");
                        return;
                    }
                }

                if (progress.IsCancelled())
                {
                    AICLI_LOG(Repo, Info, << "Cancelling update upon request");
                    return;
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
                    SourceTrustLevel::Trusted == details.TrustLevel,
                    progress);

                if (download)
                {
                    // If successful, delete the file
                    std::filesystem::remove(tempFile);
                }
            }

            void RemoveInternal(const SourceDetails& details, IProgressCallback& callback) override
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
            }
        };

        // Source factory for running outside of a package.
        struct DesktopContextFactory : public PreIndexedFactoryBase
        {
            // Constructs the location that we will write files to.
            std::filesystem::path GetStatePathFromDetails(const SourceDetails& details)
            {
                std::filesystem::path result = Runtime::GetPathTo(Runtime::PathName::LocalState);
                result /= PreIndexedPackageSourceFactory::Type();
                result /= GetPackageFamilyNameFromDetails(details);
                return result;
            }

            std::shared_ptr<ISource> CreateInternal(const SourceDetails& details, Synchronization::CrossProcessReaderWriteLock&& lock, IProgressCallback&) override
            {
                std::filesystem::path packageLocation = GetStatePathFromDetails(details);
                packageLocation /= s_PreIndexedPackageSourceFactory_IndexFileName;

                if (!std::filesystem::exists(packageLocation))
                {
                    AICLI_LOG(Repo, Info, << "Data not found at " << packageLocation);
                    THROW_HR(APPINSTALLER_CLI_ERROR_SOURCE_DATA_MISSING);
                }

                SQLiteIndex index = SQLiteIndex::Open(packageLocation.u8string(), SQLiteIndex::OpenDisposition::Read);

                // We didn't use to store the source identifier, so we compute it here in case it's
                // missing from the details.
                return std::make_shared<SQLiteIndexSource>(details, GetPackageFamilyNameFromDetails(details), std::move(index), std::move(lock));
            }

            void UpdateInternal(std::string packageLocation, const SourceDetails& details, IProgressCallback& progress) override
            {
                // We will extract the manifest and index files directly to this location
                std::filesystem::path packageState = GetStatePathFromDetails(details);
                std::filesystem::create_directories(packageState);

                Msix::MsixInfo packageInfo(packageLocation);
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_PACKAGE_IS_BUNDLE, packageInfo.GetIsBundle());

                if (progress.IsCancelled())
                {
                    AICLI_LOG(Repo, Info, << "Cancelling update upon request");
                    return;
                }

                std::filesystem::path manifestPath = packageState / s_PreIndexedPackageSourceFactory_AppxManifestFileName;
                std::filesystem::path indexPath = packageState / s_PreIndexedPackageSourceFactory_IndexFileName;

                if (std::filesystem::exists(manifestPath) && std::filesystem::exists(indexPath))
                {
                    // If we already have a manifest, use it to determine if we need to update or not.
                    if (!packageInfo.IsNewerThan(manifestPath))
                    {
                        AICLI_LOG(Repo, Info, << "Remote source data was not newer than existing, no update needed");
                        return;
                    }
                }

                if (progress.IsCancelled())
                {
                    AICLI_LOG(Repo, Info, << "Cancelling update upon request");
                    return;
                }

                packageInfo.WriteToFile(s_PreIndexedPackageSourceFactory_IndexFilePath, indexPath, progress);
                packageInfo.WriteManifestToFile(manifestPath, progress);
            }

            void RemoveInternal(const SourceDetails& details, IProgressCallback&) override
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
