// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "UninstallFlow.h"
#include "WorkflowBase.h"
#include "ShellExecuteInstallerHandler.h"
#include "AppInstallerMsixInfo.h"

#include <AppInstallerDeployment.h>
#include <winget/PackageTrackingCatalog.h>

using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Msix;
using namespace AppInstaller::Repository;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        // Helper for RecordUninstall
        struct UninstallCorrelatedSources
        {
            struct Item
            {
                Utility::LocIndString Identifier;
                std::shared_ptr<const ISource> Source;
                std::string SourceIdentifier;
            };

            void AddIfRemoteAndNotPresent(const std::shared_ptr<IPackageVersion>& packageVersion)
            {
                auto source = packageVersion->GetSource();
                const auto& details = source->GetDetails();
                if (!ContainsAvailablePackages(details.Origin))
                {
                    return;
                }

                for (const auto& item : Items)
                {
                    if (item.SourceIdentifier == details.Identifier)
                    {
                        return;
                    }
                }

                Items.emplace_back(Item{ packageVersion->GetProperty(PackageVersionProperty::Id), std::move(source), details.Identifier });
            }

            std::vector<Item> Items;
        };
    }

    void GetUninstallInfo(Execution::Context& context)
    {
        auto installedPackageVersion = context.Get<Execution::Data::InstalledPackageVersion>();
        const std::string installedTypeString = installedPackageVersion->GetMetadata()[PackageVersionMetadata::InstalledType];
        switch (ConvertToInstallerTypeEnum(installedTypeString))
        {
        case InstallerTypeEnum::Exe:
        case InstallerTypeEnum::Burn:
        case InstallerTypeEnum::Inno:
        case InstallerTypeEnum::Nullsoft:
        {
            IPackageVersion::Metadata packageMetadata = installedPackageVersion->GetMetadata();

            // Default to silent unless it is not present or interactivity is requested
            auto uninstallCommandItr = packageMetadata.find(PackageVersionMetadata::SilentUninstallCommand);
            if (uninstallCommandItr == packageMetadata.end() || context.Args.Contains(Execution::Args::Type::Interactive))
            {
                auto interactiveItr = packageMetadata.find(PackageVersionMetadata::StandardUninstallCommand);
                if (interactiveItr != packageMetadata.end())
                {
                    uninstallCommandItr = interactiveItr;
                }
            }

            if (uninstallCommandItr == packageMetadata.end())
            {
                context.Reporter.Error() << Resource::String::NoUninstallInfoFound << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_UNINSTALL_INFO_FOUND);
            }

            context.Add<Execution::Data::UninstallString>(uninstallCommandItr->second);
            break;
        }
        case InstallerTypeEnum::Msi:
        case InstallerTypeEnum::Wix:
        {
            // Uninstall strings for MSI don't include UI level (/q) needed to avoid interactivity,
            // so we handle them differently.
            auto productCodes = installedPackageVersion->GetMultiProperty(PackageVersionMultiProperty::ProductCode);
            if (productCodes.empty())
            {
                context.Reporter.Error() << Resource::String::NoUninstallInfoFound << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_UNINSTALL_INFO_FOUND);
            }

            context.Add<Execution::Data::ProductCodes>(std::move(productCodes));
            break;
        }
        case InstallerTypeEnum::Msix:
        case InstallerTypeEnum::MSStore:
        {
            auto packageFamilyNames = installedPackageVersion->GetMultiProperty(PackageVersionMultiProperty::PackageFamilyName);
            if (packageFamilyNames.empty())
            {
                context.Reporter.Error() << Resource::String::NoUninstallInfoFound << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_UNINSTALL_INFO_FOUND);
            }

            context.Add<Execution::Data::PackageFamilyNames>(packageFamilyNames);
            break;
        }
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }

    void ExecuteUninstaller(Execution::Context& context)
    {
        const std::string installedTypeString = context.Get<Execution::Data::InstalledPackageVersion>()->GetMetadata()[PackageVersionMetadata::InstalledType];
        switch (ConvertToInstallerTypeEnum(installedTypeString))
        {
        case InstallerTypeEnum::Exe:
        case InstallerTypeEnum::Burn:
        case InstallerTypeEnum::Inno:
        case InstallerTypeEnum::Nullsoft:
            context << Workflow::ShellExecuteUninstallImpl;
            break;
        case InstallerTypeEnum::Msi:
        case InstallerTypeEnum::Wix:
            context << Workflow::ShellExecuteMsiExecUninstall;
            break;
        case InstallerTypeEnum::Msix:
        case InstallerTypeEnum::MSStore:
            context << Workflow::MsixUninstall;
            break;
        default:
        THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }

    void MsixUninstall(Execution::Context& context)
    {
        const auto& packageFamilyNames = context.Get<Execution::Data::PackageFamilyNames>();
        context.Reporter.Info() << Resource::String::UninstallFlowStartingPackageUninstall << std::endl;

        for (const auto& packageFamilyName : packageFamilyNames)
        {
            auto packageFullName = Msix::GetPackageFullNameFromFamilyName(packageFamilyName);
            if (!packageFullName.has_value())
            {
                AICLI_LOG(CLI, Warning, << "No package found with family name: " << packageFamilyName);
                continue;
            }

            AICLI_LOG(CLI, Info, << "Removing MSIX package: " << packageFullName.value());
            context.Reporter.ExecuteWithProgress(std::bind(Deployment::RemovePackage, packageFullName.value(), std::placeholders::_1));
        }

        context.Reporter.Info() << Resource::String::UninstallFlowUninstallSuccess << std::endl;
    }

    void RecordUninstall(Context& context)
    {
        // In order to report an uninstall to every correlated tracking catalog, we first need to find them all.
        auto package = context.Get<Data::Package>();
        UninstallCorrelatedSources correlatedSources;

        // Start with the installed version
        correlatedSources.AddIfRemoteAndNotPresent(package->GetInstalledVersion());

        // Then look through all available versions
        for (const auto& versionKey : package->GetAvailableVersionKeys())
        {
            correlatedSources.AddIfRemoteAndNotPresent(package->GetAvailableVersion(versionKey));
        }

        // Finally record the uninstall for each found value
        for (const auto& item : correlatedSources.Items)
        {
            auto trackingCatalog = PackageTrackingCatalog::CreateForSource(item.Source);
            trackingCatalog.RecordUninstall(item.Identifier);
        }
    }
}
