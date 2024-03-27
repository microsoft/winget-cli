// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RepairFlow.h"
#include "Workflows/ShellExecuteInstallerHandler.h"
#include "Workflows/WorkflowBase.h"
#include "Workflows/DownloadFlow.h"
#include "Workflows/ArchiveFlow.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/PromptFlow.h"
#include "winget/ManifestCommon.h"
#include "AppInstallerDeployment.h"
#include "AppInstallerMsixInfo.h"
#include "AppInstallerSynchronization.h"
#include "MSStoreInstallerHandler.h"
#include "ManifestComparator.h"
#include <winget/PackageVersionSelection.h>

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Msix;
using namespace AppInstaller::Repository;

namespace AppInstaller::CLI::Workflow
{
    // Internal implementation details
    namespace
    {
        // Sets the uninstall string in the context.
        // RequiredArgs:
        // Inputs:InstalledPackageVersion
        // Outputs:SilentUninstallString, UninstallString
        void SetUninstallStringInContext(Execution::Context& context)
        {
            const auto& installedPackageVersion = context.Get<Execution::Data::InstalledPackageVersion>();
            IPackageVersion::Metadata packageMetadata = installedPackageVersion->GetMetadata();

            // Default to silent unless it is not present or interactivity is requested
            auto uninstallCommandItr = packageMetadata.find(PackageVersionMetadata::SilentUninstallCommand);

            if ((!context.Args.Contains(Execution::Args::Type::Silent) && uninstallCommandItr == packageMetadata.end())
                || context.Args.Contains(Execution::Args::Type::Interactive))
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

        // Sets the modify path in the context.
        // RequiredArgs:None
        // Inputs:InstalledPackageVersion
        // Outputs:ModifyPath
        void SetModifyPathInContext(Execution::Context& context)
        {
            const auto& installedPackageVersion = context.Get<Execution::Data::InstalledPackageVersion>();
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

        // Sets the product codes in the context.
        // RequiredArgs:None
        // Inputs:InstalledPackageVersion
        // Outputs:ProductCodes
        void SetProductCodesInContext(Execution::Context& context)
        {
            const auto& installedPackageVersion = context.Get<Execution::Data::InstalledPackageVersion>();
            auto productCodes = installedPackageVersion->GetMultiProperty(PackageVersionMultiProperty::ProductCode);

            if (productCodes.empty())
            {
                context.Reporter.Error() << Resource::String::NoRepairInfoFound << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_REPAIR_INFO_FOUND);
            }

            context.Add<Execution::Data::ProductCodes>(productCodes);
        }

        // Sets the package family names in the context.
        // RequiredArgs:None
        // Inputs:InstalledPackageVersion
        // Outputs:PackageFamilyNames
        void SetPackageFamilyNamesInContext(Execution::Context& context)
        {
            const auto& installedPackageVersion = context.Get<Execution::Data::InstalledPackageVersion>();

            auto packageFamilyNames = installedPackageVersion->GetMultiProperty(PackageVersionMultiProperty::PackageFamilyName);
            if (packageFamilyNames.empty())
            {
                context.Reporter.Error() << Resource::String::NoRepairInfoFound << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_REPAIR_INFO_FOUND);
            }

            context.Add<Execution::Data::PackageFamilyNames>(packageFamilyNames);
        }

        // The function performs a preliminary check on the installed package by reading its ARP registry flags for NoModify and NoRepair to confirm if the repair operation is applicable.
        // RequiredArgs:None
        // Inputs:InstalledPackageVersion, NoModify ?, NoRepair ?
        // Outputs:None
        void ApplicabilityCheckForInstalledPackage(Execution::Context& context)
        {
            // Installed Package repair applicability check
            const auto& installedPackageVersion = context.Get<Execution::Data::InstalledPackageVersion>();

            const std::string installerType = context.Get<Execution::Data::InstalledPackageVersion>()->GetMetadata()[PackageVersionMetadata::InstalledType];
            InstallerTypeEnum installerTypeEnum = ConvertToInstallerTypeEnum(installerType);

            if (installerTypeEnum == InstallerTypeEnum::Portable || installerTypeEnum == InstallerTypeEnum::Unknown)
            {
                context.Reporter.Error() << Resource::String::RepairOperationNotSupported << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_REPAIR_NOT_SUPPORTED);
            }

            IPackageVersion::Metadata packageMetadata = installedPackageVersion->GetMetadata();

            auto noModifyItr = packageMetadata.find(PackageVersionMetadata::NoModify);
            std::string noModifyARPFlag = noModifyItr != packageMetadata.end() ? noModifyItr->second : std::string();

            auto noRepairItr = packageMetadata.find(PackageVersionMetadata::NoRepair);
            std::string noRepairARPFlag = noRepairItr != packageMetadata.end() ? noRepairItr->second : std::string();

            if (Utility::IsDwordFlagSet(noModifyARPFlag) || Utility::IsDwordFlagSet(noRepairARPFlag))
            {
                context.Reporter.Error() << Resource::String::RepairOperationNotSupported << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_REPAIR_NOT_SUPPORTED);
            }
        }

        // This function performs a preliminary check on the available matching package by reading its manifest entries for repair behavior to determine the type of repair operation and repair switch are applicable
        // RequiredArgs:None
        // Inputs:InstallerType, RepairBehavior
        // Outputs:None
        void ApplicabilityCheckForAvailablePackage(Execution::Context& context)
        {
            // Selected Installer repair applicability check
            auto installerType = context.Get<Execution::Data::Installer>()->EffectiveInstallerType();
            auto repairBehavior = context.Get<Execution::Data::Installer>()->RepairBehavior;

            if (installerType == InstallerTypeEnum::Portable || installerType == InstallerTypeEnum::Unknown)
            {
                context.Reporter.Error() << Resource::String::RepairOperationNotSupported << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_REPAIR_NOT_SUPPORTED);
            }

            // Repair behavior is required for Burn, Inno, Nullsoft, Exe installers
            if (DoesInstallerTypeRequireRepairBehaviorForRepair(installerType) &&
                repairBehavior == RepairBehaviorEnum::Unknown)
            {
                context.Reporter.Error() << Resource::String::NoRepairInfoFound << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_REPAIR_INFO_FOUND);
            }
        }

        // Generate the repair string based on the repair behavior and installer type.
        // RequiredArgs:None
        // Inputs:BaseInstallerType, RepairBehavior, ModifyPath?, UninstallString?, InstallerArgs
        // Outputs:RepairString
        void GenerateRepairString(Execution::Context& context)
        {
            const auto& installer = context.Get<Execution::Data::Installer>();
            auto installerType = installer->BaseInstallerType;
            auto repairBehavior = installer->RepairBehavior;

            std::string repairCommand;

            switch (repairBehavior)
            {
            case RepairBehaviorEnum::Modify:
            {
                SetModifyPathInContext(context);
                repairCommand.append(context.Get<Execution::Data::ModifyPath>());
            }
            break;
            case RepairBehaviorEnum::Installer:
            {
                // [NOTE:] We will ShellExecuteInstall for this scenario which uses installer path directly.so no need for repair command generation.
                // We prepare installer download and archive extraction here.
                context <<
                    ShowInstallationDisclaimer <<
                    ShowPromptsForSinglePackage(/* ensureAcceptance */ true) <<
                    DownloadInstaller;

                if (installerType == InstallerTypeEnum::Zip)
                {
                    context <<
                        ScanArchiveFromLocalManifest <<
                        ExtractFilesFromArchive <<
                        VerifyAndSetNestedInstaller;
                }
            }
            break;
            case RepairBehaviorEnum::Uninstaller:
            {
                SetUninstallStringInContext(context);
                repairCommand.append(context.Get<Execution::Data::UninstallString>());
            }
            break;
            case RepairBehaviorEnum::Unknown:
            default:
                context.Reporter.Error() << Resource::String::NoRepairInfoFound << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_REPAIR_INFO_FOUND);
            }

            context <<
                GetInstallerArgs;

            // Following is not applicable for RepairBehaviorEnum::Installer, as we can call ShellExecuteInstall directly with repair argument.
            if (repairBehavior != RepairBehaviorEnum::Installer)
            {
                if (repairCommand.empty())
                {
                    context.Reporter.Error() << Resource::String::NoRepairInfoFound << std::endl;
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_REPAIR_INFO_FOUND);
                }

                repairCommand.append(" ");
                repairCommand.append(context.Get<Execution::Data::InstallerArgs>());
                context.Add<Execution::Data::RepairString>(repairCommand);
            }
        }
    }

    void RunRepairForRepairBehaviorBasedInstaller(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>();
        auto repairBehavior = installer->RepairBehavior;

        if (repairBehavior == RepairBehaviorEnum::Modify || repairBehavior == RepairBehaviorEnum::Uninstaller)
        {
            context <<
                ShellExecuteRepairImpl <<
                ReportRepairResult(RepairBehaviorToString(repairBehavior), APPINSTALLER_CLI_ERROR_EXEC_REPAIR_FAILED);
        }
        else if (repairBehavior == RepairBehaviorEnum::Installer)
        {
            context <<
                ShellExecuteInstallImpl <<
                ReportInstallerResult(RepairBehaviorToString(repairBehavior), APPINSTALLER_CLI_ERROR_EXEC_REPAIR_FAILED);
        }
        else
        {
            context.Reporter.Error() << Resource::String::NoRepairInfoFound << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_REPAIR_INFO_FOUND);
        }
    }

    void RepairMsiBasedInstaller(Execution::Context& context)
    {
        context <<
            ShellExecuteMsiExecRepair <<
            ReportRepairResult("MsiExec", APPINSTALLER_CLI_ERROR_EXEC_REPAIR_FAILED);
    }

    void RepairApplicabilityCheck(Execution::Context& context)
    {
        context <<
            ApplicabilityCheckForInstalledPackage <<
            ApplicabilityCheckForAvailablePackage;
    }

    void ExecuteRepair(Execution::Context& context)
    {
        // [TODO:] At present, the repair flow necessitates a mapped available installer.
        // However, future refactoring should allow for msix/msi repair without the need for one.

        const auto& installer = context.Get<Execution::Data::Installer>();
        InstallerTypeEnum installerTypeEnum = installer->EffectiveInstallerType();

        Synchronization::CrossProcessInstallLock lock;

        if (!ExemptFromSingleInstallLocking(installerTypeEnum))
        {
            // Acquire the lock , if the operation is cancelled it will return false so we will also return.
            if (!context.Reporter.ExecuteWithProgress([&](IProgressCallback& callback)
                {
                    callback.SetProgressMessage(Resource::String::InstallWaitingOnAnother());
                    return lock.Acquire(callback);
                }))
            {
                AICLI_LOG(CLI, Info, << "Abandoning attempt to acquire repair lock due to cancellation");
                return;
            }
        }

        switch (installerTypeEnum)
        {
        case InstallerTypeEnum::Burn:
        case InstallerTypeEnum::Exe:
        case InstallerTypeEnum::Inno:
        case InstallerTypeEnum::Nullsoft:
        {
            context <<
                RunRepairForRepairBehaviorBasedInstaller;
        }
        break;
        case InstallerTypeEnum::Msi:
        case InstallerTypeEnum::Wix:
        {
            context <<
                RepairMsiBasedInstaller;
        }
        break;
        case InstallerTypeEnum::Msix:
        {
            context <<
                RepairMsixPackage;
        }
        break;
        case InstallerTypeEnum::MSStore:
        {
            context <<
                EnsureStorePolicySatisfied <<
                MSStoreRepair;
        }
        break;
        case InstallerTypeEnum::Portable:
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }

    void GetRepairInfo(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>();
        InstallerTypeEnum installerTypeEnum = installer->EffectiveInstallerType();

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
        {
            context <<
                SetProductCodesInContext;
        }
        break;
        case InstallerTypeEnum::Msix:
        {
            context <<
                SetPackageFamilyNamesInContext;
        }
        break;
        case InstallerTypeEnum::MSStore:
            break;
        case InstallerTypeEnum::Portable:
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
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

    void RepairSinglePackage(Execution::Context& context)
    {
        context <<
            RepairApplicabilityCheck <<
            GetRepairInfo <<
            ReportExecutionStage(ExecutionStage::Execution) <<
            ExecuteRepair <<
            ReportExecutionStage(ExecutionStage::PostExecution);
    }

    void SelectApplicablePackageVersion(Execution::Context& context)
    {
        const auto& installedPackage = context.Get<Execution::Data::InstalledPackageVersion>();

        Utility::Version installedVersion = Utility::Version(installedPackage->GetProperty(PackageVersionProperty::Version));
        if (installedVersion.IsUnknown())
        {
            context.Reporter.Error() << Resource::String::NoApplicableInstallers << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER);
        }

        std::string_view requestedVersion = context.Args.Contains(Execution::Args::Type::TargetVersion) ? context.Args.GetArg(Execution::Args::Type::TargetVersion) : installedVersion.ToString();
        // If it's Store source with only one version unknown, use the unknown version for available version mapping.
        const auto& package = context.Get<Execution::Data::Package>();
        auto packageVersions = GetAvailableVersionsForInstalledVersion(package, installedPackage);
        auto versionKeys = packageVersions->GetVersionKeys();
        if (versionKeys.size() == 1)
        {
            auto packageVersion = packageVersions->GetVersion(versionKeys.at(0));
            if (packageVersion->GetSource().IsWellKnownSource(WellKnownSource::MicrosoftStore) &&
                Utility::Version{ packageVersion->GetProperty(PackageVersionProperty::Version) }.IsUnknown())
            {
                requestedVersion = "";
            }
        }

        context <<
            GetManifestWithVersionFromPackage(
                requestedVersion,
                context.Args.GetArg(Execution::Args::Type::Channel), false) <<
            SelectInstaller <<
            EnsureApplicableInstaller;
    }

    void ReportRepairResult::operator()(Execution::Context& context) const
    {
        DWORD repairResult = context.Get<Execution::Data::OperationReturnCode>();

        if (repairResult != 0)
        {
            const auto& repairPackage = context.Get<Execution::Data::PackageVersion>();

            Logging::Telemetry().LogRepairFailure(
                repairPackage->GetProperty(PackageVersionProperty::Id),
                repairPackage->GetProperty(PackageVersionProperty::Version),
                m_repairType,
                repairResult);

            if (m_isHResult)
            {
                context.Reporter.Error()
                    << Resource::String::RepairFailedWithCode(Utility::LocIndView{ GetUserPresentableMessage(repairResult) })
                    << std::endl;
            }
            else
            {
                context.Reporter.Error()
                    << Resource::String::RepairFailedWithCode(repairResult)
                    << std::endl;
            }

            // Show log path if available
            if (context.Contains(Execution::Data::LogPath) && std::filesystem::exists(context.Get<Execution::Data::LogPath>()))
            {
                auto installerLogPath = Utility::LocIndString{ context.Get<Execution::Data::LogPath>().u8string() };
                context.Reporter.Info() << Resource::String::InstallerLogAvailable(installerLogPath) << std::endl;
            }

            AICLI_TERMINATE_CONTEXT(m_hr);
        }
        else
        {
            context.Reporter.Info() << Resource::String::RepairFlowRepairSuccess << std::endl;
        }
    }
}