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
        switch (ManifestInstaller::ConvertToInstallerTypeEnum(installedTypeString))
        {
        case ManifestInstaller::InstallerTypeEnum::Exe:
        case ManifestInstaller::InstallerTypeEnum::Burn:
        case ManifestInstaller::InstallerTypeEnum::Inno:
        case ManifestInstaller::InstallerTypeEnum::Msi:
        case ManifestInstaller::InstallerTypeEnum::Nullsoft:
        case ManifestInstaller::InstallerTypeEnum::Wix:
        {
            IPackageVersion::Metadata packageMetadata = installedPackageVersion->GetMetadata();
            IPackageVersion::Metadata::iterator itr = packageMetadata.find(PackageVersionMetadata::StandardUninstallCommand);

            if (itr == packageMetadata.end() || context.Args.Contains(Execution::Args::Type::Silent))
            {
                itr = packageMetadata.find(PackageVersionMetadata::SilentUninstallCommand);
            }

            if (itr == packageMetadata.end())
            {
                context.Reporter.Error() << Resource::String::NoUninstallInfoFound << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_UNINSTALL_INFO_FOUND);
            }

            context.Add<Execution::Data::UninstallString>(itr->second);
            break;
        }
        case ManifestInstaller::InstallerTypeEnum::Msix:
        case ManifestInstaller::InstallerTypeEnum::MSStore:
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
        switch (ManifestInstaller::ConvertToInstallerTypeEnum(installedTypeString))
        {
        case ManifestInstaller::InstallerTypeEnum::Exe:
        case ManifestInstaller::InstallerTypeEnum::Burn:
        case ManifestInstaller::InstallerTypeEnum::Inno:
        case ManifestInstaller::InstallerTypeEnum::Msi:
        case ManifestInstaller::InstallerTypeEnum::Nullsoft:
        case ManifestInstaller::InstallerTypeEnum::Wix:
            context << Workflow::ShellExecuteUninstallImpl;
            break;
        case ManifestInstaller::InstallerTypeEnum::Msix:
        case ManifestInstaller::InstallerTypeEnum::MSStore:
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
            if (packageFullName.has_value())
            {
                AICLI_LOG(CLI, Info, << "Removing MSIX package: " << packageFullName.value());
                context.Reporter.ExecuteWithProgress(std::bind(Deployment::RemovePackage, packageFullName.value(), std::placeholders::_1));
            }
        }

        context.Reporter.Info() << Resource::String::UninstallFlowUninstallSuccess << std::endl;
    }
}