// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Microsoft/PreIndexedPackageSourceFactory.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace AppInstaller::Repository::Microsoft
{
    namespace
    {
        // TODO: Insert more final name here when available
        static constexpr std::string_view s_PreIndexedPackageSourceFactory_PackageFileName = "AppInstallerSQLLiteIndex.msix"sv;

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
            bool IsInitialized(const SourceDetails& details) override final
            {
                return !details.Data.empty();
            }

            std::unique_ptr<ISource> Create(const SourceDetails& details) override final
            {
                THROW_HR_IF(E_INVALIDARG, details.Type != PreIndexedPackageSourceFactory::Type());

                auto lock = Synchronization::CrossProcessReaderWriteLock::LockForRead(CreateNameForCPRWL(details));
                return CreateInternal(details, std::move(lock));
            }

            virtual std::unique_ptr<ISource> CreateInternal(const SourceDetails& details, Synchronization::CrossProcessReaderWriteLock&& lock) = 0;

            void Update(SourceDetails& details, IProgressCallback& progress) override final
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

                if (!IsInitialized(details))
                {
                    AICLI_LOG(Repo, Info, << "Initializing source from: " << details.Name << " => " << packageLocation);

                    // If not initialized, we need to open the package and get the family name.
                    Msix::MsixInfo packageInfo(packageLocation);
                    THROW_HR_IF(APPINSTALLER_CLI_ERROR_PACKAGE_IS_BUNDLE, packageInfo.GetIsBundle());
                    details.Data = packageInfo.GetPackageFamilyName();

                    AICLI_LOG(Repo, Info, << "Found package family name: " << details.Name << " => " << details.Data);
                }

                auto lock = Synchronization::CrossProcessReaderWriteLock::LockForWrite(CreateNameForCPRWL(details));

                UpdateInternal(packageLocation, details, progress);

                details.LastUpdateTime = std::chrono::system_clock::now();
            }

            virtual void UpdateInternal(std::string packageLocation, SourceDetails& details, IProgressCallback& progress) = 0;

            void Remove(const SourceDetails& details, IProgressCallback& progress) override final
            {
                using namespace std::chrono_literals;

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
            auto GetPackageFromDetails(const SourceDetails& details)
            {
                using Package = winrt::Windows::ApplicationModel::Package;

                std::wstring packageFamilyName = Utility::ConvertToUTF16(GetPackageFamilyNameFromDetails(details));

                Package currentPackage = Package::Current();
                auto dependencies = currentPackage.Dependencies();
                for (uint32_t i = 0; i < dependencies.Size(); ++i)
                {
                    Package package = dependencies.GetAt(i);
                    if (package.Id().FamilyName() == packageFamilyName)
                    {
                        if (package.IsOptional())
                        {
                            return package;
                        }
                        else
                        {
                            AICLI_LOG(Repo, Error, << "Source references a non-optional package: " << details.Name << " => " << GetPackageFamilyNameFromDetails(details));
                            return Package{ nullptr };
                        }
                    }
                }
                AICLI_LOG(Repo, Error, << "Source references an unknown package: " << details.Name << " => " << GetPackageFamilyNameFromDetails(details));
                return Package{ nullptr };
            }

            std::unique_ptr<ISource> CreateInternal(const SourceDetails& details, Synchronization::CrossProcessReaderWriteLock&& lock) override
            {
                UNREFERENCED_PARAMETER(details);
                UNREFERENCED_PARAMETER(lock);
                THROW_HR(E_NOTIMPL);
            }

            void UpdateInternal(std::string packageLocation, SourceDetails&, IProgressCallback& progress) override
            {
                winrt::Windows::Foundation::Uri uri(Utility::ConvertToUTF16(packageLocation));
                Deployment::RequestAddPackageAsync(uri, winrt::Windows::Management::Deployment::DeploymentOptions::None, progress);
            }

            void RemoveInternal(const SourceDetails& details, IProgressCallback&) override
            {
                // Get the package referenced by the details
                auto optionalPackage = GetPackageFromDetails(details);
                if (!optionalPackage)
                {
                    AICLI_LOG(Repo, Info, << "Package not found by family name " << details.Data);
                    return;
                }

                // Begin package removal, but let it run its course without waiting.
                // This pattern is required due to the inability to use SetInUseAsync from a full trust process.
                AICLI_LOG(Repo, Info, << "Removing package " << Utility::ConvertToUTF8(optionalPackage.Id().FullName()));
                Deployment::RemovePackageFireAndForget(optionalPackage.Id().FullName());
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
            THROW_HR(E_NOTIMPL);
        }
    }
}
