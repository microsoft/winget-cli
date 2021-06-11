// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "UninstallFlow.h"
#include "WorkflowBase.h"
#include "ShellExecuteInstallerHandler.h"
#include "AppInstallerMsixInfo.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Msix;
using namespace AppInstaller::Repository;

namespace AppInstaller::CLI::Workflow
{
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

    void ReportDependencies(Execution::Context& context)
    {
        context <<
            Workflow::GetManifest <<
            Workflow::SelectInstaller;
            
        const auto& installer = context.Get<Execution::Data::Installer>();
        if (installer)
        {
            auto dependencies = installer->Dependencies;
            if (dependencies.HasAny())
            {
                context.Reporter.Info() << "This package had dependencies that may not be needed anymore:" << std::endl;

                auto windowsFeaturesDep = dependencies.WindowsFeatures;
                if (!windowsFeaturesDep.empty())
                {
                    context.Reporter.Info() << "    - Windows Features: ";
                    for (size_t i = 0; i < windowsFeaturesDep.size(); i++)
                    {
                        context.Reporter.Info() << windowsFeaturesDep[i];
                        if (i < windowsFeaturesDep.size() - 1) context.Reporter.Info() << ", ";
                    }
                    context.Reporter.Info() << std::endl;
                }

                auto windowsLibrariesDep = dependencies.WindowsLibraries;
                if (!windowsLibrariesDep.empty())
                {
                    context.Reporter.Info() << "    - Windows Libraries: ";
                    for (size_t i = 0; i < windowsLibrariesDep.size(); i++)
                    {
                        context.Reporter.Info() << windowsLibrariesDep[i];
                        if (i < windowsLibrariesDep.size() - 1) context.Reporter.Info() << ", ";
                    }
                    context.Reporter.Info() << std::endl;
                }
                
                auto packageDep = dependencies.PackageDependencies;
                if (!packageDep.empty())
                {
                    context.Reporter.Info() << "    - Packages: ";
                    for (size_t i = 0; i < packageDep.size(); i++)
                    {
                        context.Reporter.Info() << packageDep[i].Id;
                        if (!packageDep[i].MinVersion.empty()) context.Reporter.Info() << " [>= " << packageDep[i].MinVersion << "]";
                        if (i < packageDep.size() - 1) context.Reporter.Info() << ", ";
                    }
                    context.Reporter.Info() << std::endl;
                }

                auto externalDependenciesDep = dependencies.ExternalDependencies;
                if (!externalDependenciesDep.empty())
                {
                    context.Reporter.Info() << "    - Externals: ";
                    for (size_t i = 0; i < externalDependenciesDep.size(); i++)
                    {
                        context.Reporter.Info() << externalDependenciesDep[i];
                        if (i < externalDependenciesDep.size() - 1) context.Reporter.Info() << ", ";
                    }
                    context.Reporter.Info() << std::endl;
                }
            }
        }
    }
}