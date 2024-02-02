// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RepairFlow.h"
#include "Workflows/ShellExecuteInstallerHandler.h"
#include "Workflows/WorkflowBase.h"
#include "Workflows/DownloadFlow.h"
#include "Workflows/ArchiveFlow.h"
#include "AppInstallerDeployment.h"
#include "AppInstallerMsixInfo.h"
#include "MSStoreInstallerHandler.h"
#include "ManifestComparator.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Msix;
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
            context <<
                Workflow::ShellExecuteMsiExecRepair <<
                ReportRepairResult("MsiExec", APPINSTALLER_CLI_ERROR_EXEC_REPAIR_FAILED);

            break;
        case InstallerTypeEnum::Msix:
            context <<
                Workflow::RepairMsixPackage;

            break;
        case InstallerTypeEnum::MSStore:
        {
            context <<
                Workflow::MSStoreRepair;
        }

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
            context <<
                GenerateRepairString;
        }
        break;
        case InstallerTypeEnum::Msi:
        case InstallerTypeEnum::Wix:
            context << SetProductCodesInContext;
            break;
        case InstallerTypeEnum::Msix:
        case InstallerTypeEnum::MSStore:
            context <<
                SetPackageFamilyNamesInContext;
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

    void RepairMsixPackage(Execution::Context& context)
    {
        bool isMachineScope = Manifest::ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope)) == Manifest::ScopeEnum::Machine;

        const auto& packageFamilyNames = context.Get<Execution::Data::PackageFamilyNames>();
        context.Reporter.Info() << Resource::String::RepairFlowStartingPackageRepair << std::endl;

        for (const auto& packageFamilyName : packageFamilyNames)
        {
            auto packageFullName = Msix::GetPackageFullNameFromFamilyName(packageFamilyName);

            if (!packageFullName.has_value())
            {
                AICLI_LOG(CLI, Warning, << "No package found with family name: " << packageFamilyName);
                continue;
    }

            AICLI_LOG(CLI, Info, << "Repairing package: " << packageFullName.value());

            try
            {
                if (!isMachineScope)
                {
                    // Best effort repair by registering the package.
                    context.Reporter.ExecuteWithProgress(std::bind(Deployment::RegisterPackage, packageFamilyName, std::placeholders::_1));
                }
                else
                {
                    // TODO: Do we need a different error code for repair failure?
                    // resuse install failure for the scenario.
                    context.Reporter.Error() << Resource::String::RepairFlowReturnCodeSystemNotSupported << std::endl;
                    context.Add<Execution::Data::OperationReturnCode>(static_cast<DWORD>(APPINSTALLER_CLI_ERROR_INSTALL_SYSTEM_NOT_SUPPORTED));
                    AICLI_LOG(CLI, Error, << "Device wide repair for msix type is not supported.");
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INSTALL_SYSTEM_NOT_SUPPORTED);
                }
            }
            catch (const wil::ResultException& re)
            {
                context.Add<Execution::Data::OperationReturnCode>(re.GetErrorCode());
                context << ReportRepairResult("MSIX", re.GetErrorCode(), true);
                return;
            }
        }

        context.Reporter.Info() << Resource::String::RepairFlowRepairSuccess << std::endl;
    }

    void ReportRepairResult::operator()(Execution::Context& context) const
    {
        DWORD repairResult = 0;

        if (repairResult != 0)
        {
            const auto installerPackageVersion = context.Get<Execution::Data::InstalledPackageVersion>();

            Logging::Telemetry().LogRepairFailure(
                installerPackageVersion->GetProperty(PackageVersionProperty::Id),
                installerPackageVersion->GetProperty(PackageVersionProperty::Version),
                m_repairType,
                repairResult);

            // Show log path if available
            if (context.Contains(Execution::Data::LogPath) && std::filesystem::exists(context.Get<Execution::Data::LogPath>()))
            {
                auto installerLogPath = Utility::LocIndString{ context.Get<Execution::Data::LogPath>().u8string() };
                context.Reporter.Info() << Resource::String::InstallerLogAvailable(installerLogPath) << std::endl;
        }
        }
        else
        {
            context.Reporter.Info() << Resource::String::RepairFlowRepairSuccess << std::endl;
        }
    }

    void RepairSinglePackage::operator()(Execution::Context& context) const
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), m_operationType != OperationType::Repair);

        context <<
            GetInstalledPackageVersion <<
            RepairApplicabilityCheck <<
            GetRepairInfo <<
            Workflow::ExecuteRepair;
    }

    void SelectApplicablePackageVersion::operator()(Execution::Context& context) const
    {
        auto package = context.Get<Execution::Data::Package>();
        auto installedPackage = context.Get<Execution::Data::InstalledPackageVersion>();
        const bool reportVersionNotFound = m_isSinglePackage;

        bool isRepair = WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerExecutionUseRepair);

        Utility::Version version;
        Utility::Version installedVersion;

        if (isRepair)
        {
            installedVersion = Utility::Version(installedPackage->GetProperty(PackageVersionProperty::Version));
        }

        ManifestComparator manifestComparator(context, isRepair ? installedPackage->GetMetadata() : IPackageVersion::Metadata{}); // Use the installed package metadata for comparison if we are repairing

        bool versionFound = false;
        bool installedTypeInapplicable = false;

        if (isRepair && installedVersion.IsUnknown() && !context.Args.Contains(Execution::Args::Type::IncludeUnknown))
        {
            if (reportVersionNotFound)
            {
                context.Reporter.Info() << Resource::String::NoApplicableInstallers << std::endl;
            }

            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER);
        }

        const auto& versionKeys = package->GetAvailableVersionKeys();
        bool matchingVersionFound = false;

        for (const auto& versionKey : versionKeys)
        {
            if (!matchingVersionFound || installedVersion == Utility::Version(versionKey.Version))
            {
                if (isRepair)
                {
                    matchingVersionFound = true;
                }

                auto packageVersion = package->GetAvailableVersion(versionKey);
                auto manifest = packageVersion->GetManifest();

                // Check applicable Installer
                auto [installer, inapplicabilities] = manifestComparator.GetPreferredInstaller(manifest);
                if (!installer.has_value())
                {
                    // If there is at least one installer whose only reason is InstalledType.
                    auto onlyInstalledType = std::find(inapplicabilities.begin(), inapplicabilities.end(), InapplicabilityFlags::InstalledType);
                    if (onlyInstalledType != inapplicabilities.end())
                    {
                        installedTypeInapplicable = true;
                    }

                    continue;
                }

                Logging::Telemetry().LogSelectedInstaller(
                    static_cast<int>(installer->Arch),
                    installer->Url,
                    Manifest::InstallerTypeToString(installer->EffectiveInstallerType()),
                    Manifest::ScopeToString(installer->Scope),
                    installer->Locale);

                Logging::Telemetry().LogManifestFields(
                    manifest.Id,
                    manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>(),
                    manifest.Version);

                // Since we already did installer selection, just populate the context Data
                manifest.ApplyLocale(installer->Locale);
                context.Add<Execution::Data::Manifest>(std::move(manifest));
                context.Add<Execution::Data::PackageVersion>(std::move(packageVersion));
                context.Add<Execution::Data::Installer>(std::move(installer));

                versionFound = true;
                break;
            }
            else
            {
                // Any following versions are not applicable
                break;
            }
        }

        if (!versionFound)
        {
            if (reportVersionNotFound)
            {
                if (installedTypeInapplicable)
                {
                    context.Reporter.Info() << Resource::String::RepairDifferentInstallTechnology << std::endl;
                }
                else if (isRepair)
                {
                    if (!matchingVersionFound)
                    {
                        context.Reporter.Info() << Resource::String::RepairFlowNoMatchingVersion << std::endl;
                    }
                    else
                    {
                        context.Reporter.Info() << Resource::String::RepairFlowNoApplicableVersion << std::endl;
                    }
                }
                else
                {
                    context.Reporter.Info() << Resource::String::NoApplicableInstallers << std::endl;
                }
            }

            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER);
        }
    }
}