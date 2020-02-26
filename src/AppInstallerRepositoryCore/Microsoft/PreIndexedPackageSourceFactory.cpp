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

        // Source factory for running within a packaged context
        struct PackagedContextFactory : public ISourceFactory
        {
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

            bool IsInitialized(const SourceDetails& details) override
            {
                return !details.Data.empty();
            }

            std::unique_ptr<ISource> Create(const SourceDetails& details) override
            {
                THROW_HR_IF(E_INVALIDARG, details.Type != PreIndexedPackageSourceFactory::Type());

                THROW_HR(E_NOTIMPL);
            }

            void Update(SourceDetails& details, IProgressCallback& progress) override
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

                // Get the package referenced by the details
                auto optionalPackage = GetPackageFromDetails(details);
                if (optionalPackage)
                {
                    // TODO: Actual implementation
                }
                else
                {
                    winrt::Windows::Foundation::Uri uri(Utility::ConvertToUTF16(packageLocation));
                    Deployment::RequestAddPackageAsync(uri, winrt::Windows::Management::Deployment::DeploymentOptions::None, progress);
                }
            }

            void Remove(const SourceDetails& details, IProgressCallback& progress) override
            {
                using namespace std::chrono_literals;

                THROW_HR_IF(E_INVALIDARG, details.Type != PreIndexedPackageSourceFactory::Type());
                auto lock = Synchronization::CrossProcessReaderWriteLock::LockForWrite(CreateNameForCPRWL(details));

                // Get the package referenced by the details
                auto optionalPackage = GetPackageFromDetails(details);
                if (!optionalPackage)
                {
                    return;
                }

                // Remove package
                std::vector<winrt::hstring> packageList;
                packageList.emplace_back(optionalPackage.Id().FamilyName());

                auto removeResult = Deployment::RemoveOptionalPackagesAsync(std::move(packageList), progress);
                if (FAILED(removeResult))
                {
                    AICLI_LOG(Repo, Error, << "Failed to remove package for source: " << details.Name << " => " << GetPackageFamilyNameFromDetails(details) <<
                        " [0x" << std::hex << std::setw(8) << std::setfill('0') << removeResult << "]");
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
            THROW_HR(E_NOTIMPL);
        }
    }
}
