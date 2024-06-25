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
using namespace AppInstaller::CLI::Execution;

namespace AppInstaller::CLI::Workflow
{
    // Internal implementation details
    namespace
    {

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

        InstallerTypeEnum GetInstalledType(Execution::Context& context)
        {
            const auto& installedPackage = context.Get<Execution::Data::InstalledPackageVersion>();
            std::string installedType = installedPackage->GetMetadata()[PackageVersionMetadata::InstalledType];
            return ConvertToInstallerTypeEnum(installedType);
        }

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

        void ApplicabilityCheckForAvailablePackage(Execution::Context& context)
        {
            // Skip the Available Package applicability check for MSI and MSIX repair as they aren't needed.
            if (!context.Contains(Execution::Data::Installer))
            {
                return;
            }

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

        void HandleModifyRepairBehavior(Execution::Context& context, std::string& repairCommand)
        {
            SetModifyPathInContext(context);
            repairCommand += context.Get<Execution::Data::ModifyPath>();
        }

        void HandleInstallerRepairBehavior(Execution::Context& context, InstallerTypeEnum installerType)
        {
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

        void HandleUninstallerRepairBehavior(Execution::Context& context, std::string& repairCommand)
        {
            SetUninstallStringInContext(context);
            repairCommand += context.Get<Execution::Data::UninstallString>();
        }

        void GenerateRepairString(Execution::Context& context)
        {
            const auto& installer = context.Get<Execution::Data::Installer>();
            auto installerType = installer->BaseInstallerType;
            auto repairBehavior = installer->RepairBehavior;

            std::string repairCommand;

            switch (repairBehavior)
            {
            case RepairBehaviorEnum::Modify:
                HandleModifyRepairBehavior(context, repairCommand);
                break;
            case RepairBehaviorEnum::Installer:
                HandleInstallerRepairBehavior(context, installerType);
                break;
            case RepairBehaviorEnum::Uninstaller:
                HandleUninstallerRepairBehavior(context, repairCommand);
                break;
            case RepairBehaviorEnum::Unknown:
            default:
                context.Reporter.Error() << Resource::String::NoRepairInfoFound << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_REPAIR_INFO_FOUND);
            }

            context <<
                GetInstallerArgs;

            // If the repair behavior is set to 'Installer', we can proceed with the repair command as is.
            // For repair behaviors other than 'Installer', subsequent steps will be necessary to prepare the repair command.
            if (repairBehavior == RepairBehaviorEnum::Installer)
            {
                return;
            }

            if (repairCommand.empty())
            {
                context.Reporter.Error() << Resource::String::NoRepairInfoFound << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_REPAIR_INFO_FOUND);
            }

            repairCommand += " ";
            repairCommand += context.Get<Execution::Data::InstallerArgs>();
            context.Add<Execution::Data::RepairString>(repairCommand);
        }

        bool IsInstallerMappingRequired(Execution::Context& context)
        {
            InstallerTypeEnum installerTypeEnum = GetInstalledType(context);

            switch (installerTypeEnum)
            {
            case InstallerTypeEnum::Msi:
                return false;
            case InstallerTypeEnum::Msix:
                // For MSIX packages that are from the Microsoft Store, selecting an installer is required.
                if (context.Contains(Execution::Data::Package))
                {
                    auto availablePackages = context.Get<Execution::Data::Package>()->GetAvailable();

                    if (availablePackages.size() == 1 && availablePackages[0]->GetSource() == WellKnownSource::MicrosoftStore)
                    {
                        return true;
                    }
                }

                // For MSIX packages that are not from the Microsoft Store, selecting an installer is not required.
                return false;
            default:
                return true;
            }
        }

        void HandleModifyOrUninstallerRepair(Execution::Context& context, RepairBehaviorEnum repairBehavior)
        {
            context <<
                ShellExecuteRepairImpl <<
                ReportRepairResult(RepairBehaviorToString(repairBehavior), APPINSTALLER_CLI_ERROR_EXEC_REPAIR_FAILED);
        }

        void HandleInstallerRepair(Execution::Context& context, RepairBehaviorEnum repairBehavior)
        {
            context <<
                ShellExecuteInstallImpl <<
                ReportInstallerResult(RepairBehaviorToString(repairBehavior), APPINSTALLER_CLI_ERROR_EXEC_REPAIR_FAILED);
        }
    }

    void RunRepairForRepairBehaviorBasedInstaller(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>();
        auto repairBehavior = installer->RepairBehavior;

        switch (repairBehavior)
        {
        case RepairBehaviorEnum::Modify:
        case RepairBehaviorEnum::Uninstaller:
            HandleModifyOrUninstallerRepair(context, repairBehavior);
            break;
        case RepairBehaviorEnum::Installer:
            HandleInstallerRepair(context, repairBehavior);
            break;
        default:
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
        InstallerTypeEnum installerTypeEnum = context.Contains(Execution::Data::Installer) ? context.Get<Execution::Data::Installer>()->EffectiveInstallerType() : GetInstalledType(context);

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
        case InstallerTypeEnum::Exe:
        case InstallerTypeEnum::Burn:
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
        InstallerTypeEnum installerTypeEnum = context.Contains(Execution::Data::Installer) ? context.Get<Execution::Data::Installer>()->BaseInstallerType : GetInstalledType(context);

        switch (installerTypeEnum)
        {
            // Exe based installers, for installed package all gets mapped to exe extension.
        case InstallerTypeEnum::Burn:
        case InstallerTypeEnum::Exe:
        case InstallerTypeEnum::Inno:
        case InstallerTypeEnum::Nullsoft:
        {
            context <<
                GenerateRepairString;
        }
        break;
        // MSI based installers, for installed package all gets mapped to msi extension.
        case InstallerTypeEnum::Msi:
        case InstallerTypeEnum::Wix:
        {
            context <<
                SetProductCodesInContext;
        }
        break;
        // MSIX based installers, msix.
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
        // If the repair flow is initiated with manifest, then we don't need to select the applicable package version.
        if (context.Args.Contains(Args::Type::Manifest))
        {
            return;
        }

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
                context.Args.GetArg(Execution::Args::Type::Channel), false);
    }

    void SelectApplicableInstallerIfNecessary(Execution::Context& context)
    {
        // For MSI installers, the platform provides built-in support for repair via msiexec, hence no need to select an installer.
        // Similarly, for MSIX packages that are not from the Microsoft Store, selecting an installer is not required.
        if (IsInstallerMappingRequired(context))
        {
            context <<
                SelectApplicablePackageVersion <<
                SelectInstaller <<
                EnsureApplicableInstaller;
        }
    }

    void ReportRepairResult::operator()(Execution::Context& context) const
    {
        DWORD repairResult = context.Get<Execution::Data::OperationReturnCode>();

        if (repairResult != 0)
        {
            auto& repairPackage = context.Contains(Execution::Data::PackageVersion) ?
                context.Get<Execution::Data::PackageVersion>() :
                context.Get<Execution::Data::InstalledPackageVersion>();

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
