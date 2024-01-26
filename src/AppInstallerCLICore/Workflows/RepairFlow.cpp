// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RepairFlow.h"
#include "Workflows/ShellExecuteInstallerHandler.h"
#include "Workflows/WorkflowBase.h"
#include "Workflows/DownloadFlow.h"
#include "Workflows/ArchiveFlow.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;

namespace AppInstaller::CLI::Workflow
{
    void SetUninstallStringInContext(Execution::Context& context)
    {
        auto installedPackageVersion = context.Get<Execution::Data::InstalledPackageVersion>();
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
            context.Reporter.Error() << Resource::String::NoRepairInfoFound << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_REPAIR_INFO_FOUND);
        }

        context.Add<Execution::Data::UninstallString>(uninstallCommandItr->second);
    }

    void SetModifyPathInContext(Execution::Context& context)
    {
        auto installedPackageVersion = context.Get<Execution::Data::InstalledPackageVersion>();
        IPackageVersion::Metadata packageMetadata = installedPackageVersion->GetMetadata();

        // Default to silent unless it is not present or interactivity is requested
        auto modifyPathItr = packageMetadata.find(PackageVersionMetadata::StandardModifyCommand);
        if (modifyPathItr == packageMetadata.end())
        {
            context.Reporter.Error() << Resource::String::NoRepairInfoFound << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_REPAIR_INFO_FOUND);
        }

        context.Add<Execution::Data::ModifyPath>(modifyPathItr->second);
    }

    void SetProductCodesInContext(Execution::Context& context)
    {
        auto installedPackageVersion = context.Get<Execution::Data::InstalledPackageVersion>();
        auto productCode = installedPackageVersion->GetMultiProperty(PackageVersionMultiProperty::ProductCode);

        if (productCode.empty())
        {
            context.Reporter.Error() << Resource::String::NoRepairInfoFound << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_REPAIR_INFO_FOUND);
        }

        context.Add<Execution::Data::ProductCodes>(productCode);
    }

    void SetPackageFamilyNamesInContext(Execution::Context& context)
    {
        auto installedPackageVersion = context.Get<Execution::Data::InstalledPackageVersion>();

        auto packageFamilyNames = installedPackageVersion->GetMultiProperty(PackageVersionMultiProperty::PackageFamilyName);
        if (packageFamilyNames.empty())
        {
            context.Reporter.Error() << Resource::String::NoRepairInfoFound << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_REPAIR_INFO_FOUND);
        }

        context.Add<Execution::Data::PackageFamilyNames>(packageFamilyNames);
    }

    void RepairApplicabilityCheck(Execution::Context& context)
    {
        auto installedPackageVersion = context.Get<Execution::Data::InstalledPackageVersion>();

        const std::string installerType = context.Get<Execution::Data::InstalledPackageVersion>()->GetMetadata()[PackageVersionMetadata::InstalledType];
        InstallerTypeEnum installerTypeEnum = ConvertToInstallerTypeEnum(installerType);

        if (installerTypeEnum == InstallerTypeEnum::Burn
            || installerTypeEnum == InstallerTypeEnum::Exe
            || installerTypeEnum == InstallerTypeEnum::Inno
            || installerTypeEnum == InstallerTypeEnum::Nullsoft
            || installerTypeEnum == InstallerTypeEnum::Msi
            || installerTypeEnum == InstallerTypeEnum::Wix)
        {
            IPackageVersion::Metadata packageMetadata = installedPackageVersion->GetMetadata();

            auto noModifyItr = packageMetadata.find(PackageVersionMetadata::NoModify);
            std::string noModifyARPFlag = noModifyItr != packageMetadata.end() ? noModifyItr->second : std::string();

            auto noRepairItr = packageMetadata.find(PackageVersionMetadata::NoRepair);
            std::string noRepairARPFlag = noRepairItr != packageMetadata.end() ? noRepairItr->second : std::string();

            if (Utility::IsDwordFlagSet(noModifyARPFlag) || Utility::IsDwordFlagSet(noRepairARPFlag))
            {
                // TODO: Add resource string for no repair allowed.
                THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
            }
        }
    }

    void ExecuteRepair(Execution::Context& context)
    {
        const std::string installerType = context.Get<Execution::Data::InstalledPackageVersion>()->GetMetadata()[PackageVersionMetadata::InstalledType];
        InstallerTypeEnum installerTypeEnum = ConvertToInstallerTypeEnum(installerType);

        // TODO: Obtain cross process lock to ensure only one repair is running at a time.

        switch (installerTypeEnum)
        {
        case InstallerTypeEnum::Burn:
        case InstallerTypeEnum::Exe:
        case InstallerTypeEnum::Inno:
        case InstallerTypeEnum::Nullsoft:
            //TODO: Implement Burn repair - use repair read from the manifest switch on the installer.
            context <<
                Workflow::GetInstallerArgs <<
                Workflow::ShellExecuteRepairImpl;
            break;
        case InstallerTypeEnum::Msi:
        case InstallerTypeEnum::Wix:
            // TODO: Implement Msi/Wix repair - call the msiexec.exe to repair the app
            Workflow::ShellExecuteMsiExecRepair(context);
            break;
        case InstallerTypeEnum::Msix:
            // TODO: Implement Msix repair re-register
            break;
        case InstallerTypeEnum::MSStore:
            // TODO: Implement MSStore repair - call the store API to repair the app
            break;
        case InstallerTypeEnum::Portable:
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }

    void GetRepairInfo(Execution::Context& context)
    {
        auto installedPackageVersion = context.Get<Execution::Data::InstalledPackageVersion>();
        const std::string installedTypeString = installedPackageVersion->GetMetadata()[PackageVersionMetadata::InstalledType];

        InstallerTypeEnum installerTypeEnum = ConvertToInstallerTypeEnum(installedTypeString);

        switch (installerTypeEnum)
        {
        case InstallerTypeEnum::Burn:
        case InstallerTypeEnum::Exe:
        case InstallerTypeEnum::Inno:
        case InstallerTypeEnum::Nullsoft:
        {
            context.SetFlags(Execution::ContextFlag::InstallerExecutionUseRepair);

            context <<
                Workflow::SelectInstaller << // TODO: Set ContxtFlag for repair.
                GenerateRepairString;
        }
        break;
        case InstallerTypeEnum::Msi:
        case InstallerTypeEnum::Wix:
            context << SetProductCodesInContext;
            break;
        case InstallerTypeEnum::Msix:
        case InstallerTypeEnum::MSStore:
            context << SetPackageFamilyNamesInContext;
            break;
        case InstallerTypeEnum::Portable:
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }

    void GenerateRepairString(Execution::Context& context)
    {
        auto installer = context.Get<Execution::Data::Installer>();
        //auto installerType = installer->EffectiveInstallerType();
        auto installerType = installer->BaseInstallerType;

        auto repairBehavior = installer->RepairBehavior;
        auto repairSwitch = installer->Switches[InstallerSwitchType::Repair];

        if (repairSwitch.empty())
        {
            context.Reporter.Error() << Resource::String::NoRepairInfoFound << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_REPAIR_INFO_FOUND);
        }

        std::string repairCommand;

        switch (repairBehavior)
        {
        case RepairBehaviorEnum::Modify:
            SetModifyPathInContext(context);
            repairCommand.append(context.Get<Execution::Data::ModifyPath>());

            break;
        case RepairBehaviorEnum::Installer:
            context <<
                Workflow::EnsureSupportForDownload <<
                Workflow::DownloadInstaller;

            if (installerType == InstallerTypeEnum::Zip)
            {
                context <<
                    ScanArchiveFromLocalManifest <<
                    ExtractFilesFromArchive <<
                    VerifyAndSetNestedInstaller;
            }

            // TODO: Download the installer
            // TODO: For zip installer require slightly different flow to extract the installer.

            repairCommand.append(context.Get<Execution::Data::InstallerPath>().string()); // This is the path to the downloaded installer.

            break;
        case RepairBehaviorEnum::Uninstaller:
            SetUninstallStringInContext(context);
            repairCommand.append(context.Get<Execution::Data::UninstallString>());

            break;
        case RepairBehaviorEnum::Unknown:
        default:
            context.Reporter.Error() << Resource::String::NoRepairInfoFound << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_REPAIR_INFO_FOUND);
        }

        if (repairCommand.empty())
        {
            context.Reporter.Error() << Resource::String::NoRepairInfoFound << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_REPAIR_INFO_FOUND);
        }

        repairCommand.append(" ");
        repairCommand.append(repairSwitch);

        context.Add<Execution::Data::RepairString>(repairCommand);
    }

    void RecordRepair(Execution::Context& context)
    {
        UNREFERENCED_PARAMETER(context);
    }

    void ReportRepairResult::operator()(Execution::Context& context) const
    {
        UNREFERENCED_PARAMETER(context);

        DWORD repairResult = 0;

        if (repairResult != 0)
        {
            //context.Reporter.Error() << Resource::String::RepairCommandFailure << std::endl; // TODO Add failure resource string.
        }
        else
        {
            // context.Reporter.Info() << Resource::String::RepairCommandSuccess << std::endl; // TODO:Add success resource string
        }
    }

    void RepairSinglePackage::operator()(Execution::Context& context) const
    {
        context <<
            GetInstalledPackageVersion <<
            RepairApplicabilityCheck <<
            GetRepairInfo <<
            Workflow::ExecuteRepair <<
            Workflow::RecordRepair;
    }
}