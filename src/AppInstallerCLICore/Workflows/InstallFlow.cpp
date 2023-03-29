// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallFlow.h"
#include "DownloadFlow.h"
#include "UninstallFlow.h"
#include "UpdateFlow.h"
#include "ShowFlow.h"
#include "Resources.h"
#include "ShellExecuteInstallerHandler.h"
#include "MSStoreInstallerHandler.h"
#include "MsiInstallFlow.h"
#include "ArchiveFlow.h"
#include "PortableFlow.h"
#include "WorkflowBase.h"
#include "DependenciesFlow.h"
#include "PromptFlow.h"
#include <AppInstallerMsixInfo.h>
#include <AppInstallerDeployment.h>
#include <winget/ARPCorrelation.h>
#include <winget/Archive.h>
#include <Argument.h>
#include <Command.h>
#include <AppInstallerSynchronization.h>

using namespace winrt::Windows::ApplicationModel::Store::Preview::InstallControl;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Management::Deployment;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Utility::literals;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        bool MightWriteToARP(InstallerTypeEnum type)
        {
            switch (type)
            {
            case InstallerTypeEnum::Exe:
            case InstallerTypeEnum::Burn:
            case InstallerTypeEnum::Inno:
            case InstallerTypeEnum::Msi:
            case InstallerTypeEnum::Nullsoft:
            case InstallerTypeEnum::Wix:
                return true;
            default:
                return false;
            }
        }

        bool ShouldUseDirectMSIInstall(InstallerTypeEnum type, bool isSilentInstall)
        {
            switch (type)
            {
            case InstallerTypeEnum::Msi:
            case InstallerTypeEnum::Wix:
                return isSilentInstall || ExperimentalFeature::IsEnabled(ExperimentalFeature::Feature::DirectMSI);
            default:
                return false;
            }
        }

        bool ShouldErrorForUnsupportedArgument(UnsupportedArgumentEnum arg)
        {
            switch (arg)
            {
            case UnsupportedArgumentEnum::Location:
                return true;
            default:
                return false;
            }
        }

        Execution::Args::Type GetUnsupportedArgumentType(UnsupportedArgumentEnum unsupportedArgument)
        {
            Execution::Args::Type execArg;

            switch (unsupportedArgument)
            {
            case UnsupportedArgumentEnum::Log:
                execArg = Execution::Args::Type::Log;
                break;
            case UnsupportedArgumentEnum::Location:
                execArg = Execution::Args::Type::InstallLocation;
                break;
            default:
                THROW_HR(E_UNEXPECTED);
            }

            return execArg;
        }

        struct ExpectedReturnCode
        {
            ExpectedReturnCode(ExpectedReturnCodeEnum installerReturnCode, HRESULT hr, Resource::StringId message) :
                InstallerReturnCode(installerReturnCode), HResult(hr), Message(message) {}

            static ExpectedReturnCode GetExpectedReturnCode(ExpectedReturnCodeEnum returnCode)
            {
                switch (returnCode)
                {
                case ExpectedReturnCodeEnum::PackageInUse:
                    return ExpectedReturnCode(returnCode, APPINSTALLER_CLI_ERROR_INSTALL_PACKAGE_IN_USE, Resource::String::InstallFlowReturnCodePackageInUse);
                case ExpectedReturnCodeEnum::PackageInUseByApplication:
                    return ExpectedReturnCode(returnCode, APPINSTALLER_CLI_ERROR_INSTALL_PACKAGE_IN_USE_BY_APPLICATION, Resource::String::InstallFlowReturnCodePackageInUseByApplication);
                case ExpectedReturnCodeEnum::InstallInProgress:
                    return ExpectedReturnCode(returnCode, APPINSTALLER_CLI_ERROR_INSTALL_INSTALL_IN_PROGRESS, Resource::String::InstallFlowReturnCodeInstallInProgress);
                case ExpectedReturnCodeEnum::FileInUse:
                    return ExpectedReturnCode(returnCode, APPINSTALLER_CLI_ERROR_INSTALL_FILE_IN_USE, Resource::String::InstallFlowReturnCodeFileInUse);
                case ExpectedReturnCodeEnum::MissingDependency:
                    return ExpectedReturnCode(returnCode, APPINSTALLER_CLI_ERROR_INSTALL_MISSING_DEPENDENCY, Resource::String::InstallFlowReturnCodeMissingDependency);
                case ExpectedReturnCodeEnum::DiskFull:
                    return ExpectedReturnCode(returnCode, APPINSTALLER_CLI_ERROR_INSTALL_DISK_FULL, Resource::String::InstallFlowReturnCodeDiskFull);
                case ExpectedReturnCodeEnum::InsufficientMemory:
                    return ExpectedReturnCode(returnCode, APPINSTALLER_CLI_ERROR_INSTALL_INSUFFICIENT_MEMORY, Resource::String::InstallFlowReturnCodeInsufficientMemory);
                case ExpectedReturnCodeEnum::InvalidParameter:
                    return ExpectedReturnCode(returnCode, APPINSTALLER_CLI_ERROR_INSTALL_INVALID_PARAMETER, Resource::String::InstallFlowReturnCodeInvalidParameter);
                case ExpectedReturnCodeEnum::NoNetwork:
                    return ExpectedReturnCode(returnCode, APPINSTALLER_CLI_ERROR_INSTALL_NO_NETWORK, Resource::String::InstallFlowReturnCodeNoNetwork);
                case ExpectedReturnCodeEnum::ContactSupport:
                    return ExpectedReturnCode(returnCode, APPINSTALLER_CLI_ERROR_INSTALL_CONTACT_SUPPORT, Resource::String::InstallFlowReturnCodeContactSupport);
                case ExpectedReturnCodeEnum::RebootRequiredToFinish:
                    return ExpectedReturnCode(returnCode, APPINSTALLER_CLI_ERROR_INSTALL_REBOOT_REQUIRED_TO_FINISH, Resource::String::InstallFlowReturnCodeRebootRequiredToFinish);
                case ExpectedReturnCodeEnum::RebootRequiredForInstall:
                    return ExpectedReturnCode(returnCode, APPINSTALLER_CLI_ERROR_INSTALL_REBOOT_REQUIRED_TO_INSTALL, Resource::String::InstallFlowReturnCodeRebootRequiredForInstall);
                case ExpectedReturnCodeEnum::RebootInitiated:
                    return ExpectedReturnCode(returnCode, APPINSTALLER_CLI_ERROR_INSTALL_REBOOT_INITIATED, Resource::String::InstallFlowReturnCodeRebootInitiated);
                case ExpectedReturnCodeEnum::CancelledByUser:
                    return ExpectedReturnCode(returnCode, APPINSTALLER_CLI_ERROR_INSTALL_CANCELLED_BY_USER, Resource::String::InstallFlowReturnCodeCancelledByUser);
                case ExpectedReturnCodeEnum::AlreadyInstalled:
                    return ExpectedReturnCode(returnCode, APPINSTALLER_CLI_ERROR_INSTALL_ALREADY_INSTALLED, Resource::String::InstallFlowReturnCodeAlreadyInstalled);
                case ExpectedReturnCodeEnum::Downgrade:
                    return ExpectedReturnCode(returnCode, APPINSTALLER_CLI_ERROR_INSTALL_DOWNGRADE, Resource::String::InstallFlowReturnCodeDowngrade);
                case ExpectedReturnCodeEnum::BlockedByPolicy:
                    return ExpectedReturnCode(returnCode, APPINSTALLER_CLI_ERROR_INSTALL_BLOCKED_BY_POLICY, Resource::String::InstallFlowReturnCodeBlockedByPolicy);
                case ExpectedReturnCodeEnum::SystemNotSupported:
                    return ExpectedReturnCode(returnCode, APPINSTALLER_CLI_ERROR_INSTALL_SYSTEM_NOT_SUPPORTED, Resource::String::InstallFlowReturnCodeSystemNotSupported);
                default:
                    THROW_HR(E_UNEXPECTED);
                }
            }

            ExpectedReturnCodeEnum InstallerReturnCode;
            HRESULT HResult;
            Resource::StringId Message;
        };
    }

    namespace details
    {
        // Runs the installer via ShellExecute.
        // Required Args: None
        // Inputs: Installer, InstallerPath
        // Outputs: None
        void ShellExecuteInstall(Execution::Context& context)
        {
            context <<
                GetInstallerArgs <<
                ShellExecuteInstallImpl <<
                ReportInstallerResult("ShellExecute"sv, APPINSTALLER_CLI_ERROR_SHELLEXEC_INSTALL_FAILED);
        }

        // Runs an MSI installer directly via MSI APIs.
        // Required Args: None
        // Inputs: Installer, InstallerPath
        // Outputs: None
        void DirectMSIInstall(Execution::Context& context)
        {
            context <<
                GetInstallerArgs <<
                DirectMSIInstallImpl <<
                ReportInstallerResult("MsiInstallProduct"sv, APPINSTALLER_CLI_ERROR_MSI_INSTALL_FAILED);
        }

        // Deploys the MSIX.
        // Required Args: None
        // Inputs: Manifest?, Installer || InstallerPath
        // Outputs: None
        void MsixInstall(Execution::Context& context)
        {
            std::string uri;
            if (context.Contains(Execution::Data::InstallerPath))
            {
                uri = context.Get<Execution::Data::InstallerPath>().u8string();
            }
            else
            {
                uri = context.Get<Execution::Data::Installer>()->Url;
            }

            bool isMachineScope = Manifest::ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope)) == Manifest::ScopeEnum::Machine;

            // TODO: There was a bug in deployment api if provision api was called in packaged context.
            // Remove this check when the OS bug is fixed and back ported.
            if (isMachineScope && Runtime::IsRunningInPackagedContext())
            {
                context.Reporter.Error() << Resource::String::InstallFlowReturnCodeSystemNotSupported << std::endl;
                context.Add<Execution::Data::OperationReturnCode>(static_cast<DWORD>(APPINSTALLER_CLI_ERROR_INSTALL_SYSTEM_NOT_SUPPORTED));
                AICLI_LOG(CLI, Error, << "Device wide install for msix type is not supported in packaged context.");
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INSTALL_SYSTEM_NOT_SUPPORTED);
            }

            context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

            bool registrationDeferred = false;

            try
            {
                registrationDeferred = context.Reporter.ExecuteWithProgress([&](IProgressCallback& callback)
                    {
                        if (isMachineScope)
                        {
                            return Deployment::AddPackageMachineScope(uri, callback);
                        }
                        else
                        {
                            return Deployment::AddPackageWithDeferredFallback(uri, WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerTrusted), callback);
                        }
                    });
            }
            catch (const wil::ResultException& re)
            {
                context.Add<Execution::Data::OperationReturnCode>(re.GetErrorCode());
                context << ReportInstallerResult("MSIX"sv, re.GetErrorCode(), /* isHResult */ true);
                return;
            }

            if (registrationDeferred)
            {
                context.Reporter.Warn() << Resource::String::InstallFlowRegistrationDeferred << std::endl;
            }
            else
            {
                context.Reporter.Info() << Resource::String::InstallFlowInstallSuccess << std::endl;
            }
        }

        // Runs the flow for installing a Portable package.
        // Required Args: None
        // Inputs: Installer, InstallerPath
        // Outputs: None
        void PortableInstall(Execution::Context& context)
        {
            context <<
                InitializePortableInstaller <<
                VerifyPackageAndSourceMatch <<
                PortableInstallImpl <<
                ReportInstallerResult("Portable"sv, APPINSTALLER_CLI_ERROR_PORTABLE_INSTALL_FAILED, true);
        }

        // Runs the flow for installing a package from an archive.
        // Required Args: None
        // Inputs: Installer, InstallerPath, Manifest
        // Outputs: None
        void ArchiveInstall(Execution::Context& context)
        {
            context <<
                ScanArchiveFromLocalManifest <<
                ExtractFilesFromArchive <<
                VerifyAndSetNestedInstaller <<
                ExecuteInstallerForType(context.Get<Execution::Data::Installer>().value().NestedInstallerType);
        }
    }

    bool ExemptFromSingleInstallLocking(InstallerTypeEnum type)
    {
        switch (type)
        {
            // MSStore installs are always MSIX based; MSIX installs are safe to run in parallel.
        case InstallerTypeEnum::Msix:
        case InstallerTypeEnum::MSStore:
            return true;
        default:
            return false;
        }
    }

    void EnsureApplicableInstaller(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>();

        if (!installer.has_value())
        {
            context.Reporter.Error() << Resource::String::NoApplicableInstallers << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER);
        }

        // This installer cannot be run elevated, but we are running elevated.
        // Implementation of de-elevation is complex; simply block for now.
        if (installer->ElevationRequirement == ElevationRequirementEnum::ElevationProhibited && Runtime::IsRunningAsAdmin())
        {
            context.Reporter.Error() << Resource::String::InstallerProhibitsElevation << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INSTALLER_PROHIBITS_ELEVATION);
        }

        context << EnsureSupportForInstall;
    }

    void CheckForUnsupportedArgs(Execution::Context& context)
    {
        bool messageDisplayed = false;
        const auto& unsupportedArgs = context.Get<Execution::Data::Installer>()->UnsupportedArguments;
        for (auto unsupportedArg : unsupportedArgs)
        {
            const auto& unsupportedArgType = GetUnsupportedArgumentType(unsupportedArg);
            if (context.Args.Contains(unsupportedArgType))
            {
                if (!messageDisplayed)
                {
                    context.Reporter.Warn() << Resource::String::UnsupportedArgument << std::endl;
                    messageDisplayed = true;
                }

                const auto& executingCommand = context.GetExecutingCommand();
                if (executingCommand != nullptr)
                {
                    const auto& commandArguments = executingCommand->GetArguments();
                    for (const auto& argument : commandArguments)
                    {
                        if (unsupportedArgType == argument.ExecArgType())
                        {
                            const auto& usageString = argument.GetUsageString();
                            if (ShouldErrorForUnsupportedArgument(unsupportedArg))
                            {
                                context.Reporter.Error() << usageString << std::endl;
                                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_UNSUPPORTED_ARGUMENT);
                            }
                            else
                            {
                                context.Reporter.Warn() << usageString << std::endl;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    void ShowInstallationDisclaimer(Execution::Context& context)
    {
        auto installerType = context.Get<Execution::Data::Installer>().value().EffectiveInstallerType();

        if (installerType == InstallerTypeEnum::MSStore)
        {
            context.Reporter.Info() << Execution::PromptEmphasis << Resource::String::InstallationDisclaimerMSStore << std::endl;
        }
        else
        {
            context.Reporter.Info() <<
                Resource::String::InstallationDisclaimer1 << std::endl <<
                Resource::String::InstallationDisclaimer2 << std::endl;
        }
    }

    void DisplayInstallationNotes(Execution::Context& context)
    {
        if (!Settings::User().Get<Settings::Setting::DisableInstallNotes>())
        {
            const auto& manifest = context.Get<Execution::Data::Manifest>();
            auto installationNotes = manifest.CurrentLocalization.Get<AppInstaller::Manifest::Localization::InstallationNotes>();

            if (!installationNotes.empty())
            {
                context.Reporter.Info() << Resource::String::Notes(installationNotes) << std::endl;
            }
        }
    }

    void ExecuteInstallerForType::operator()(Execution::Context& context) const
    {
        bool isUpdate = WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerExecutionUseUpdate);
        UpdateBehaviorEnum updateBehavior = context.Get<Execution::Data::Installer>().value().UpdateBehavior;
        bool doUninstallPrevious = isUpdate && (updateBehavior == UpdateBehaviorEnum::UninstallPrevious || context.Args.Contains(Execution::Args::Type::UninstallPrevious));

        Synchronization::CrossProcessInstallLock lock;
        if (!ExemptFromSingleInstallLocking(m_installerType))
        {
            // Acquire install lock; if the operation is cancelled it will return false so we will also return.
            if (!context.Reporter.ExecuteWithProgress([&](IProgressCallback& callback)
                {
                    callback.SetProgressMessage(Resource::String::InstallWaitingOnAnother());
                    return lock.Acquire(callback);
                }))
            {
                AICLI_LOG(CLI, Info, << "Abandoning attempt to acquire install lock due to cancellation");
                return;
            }
        }

        switch (m_installerType)
        {
        case InstallerTypeEnum::Exe:
        case InstallerTypeEnum::Burn:
        case InstallerTypeEnum::Inno:
        case InstallerTypeEnum::Msi:
        case InstallerTypeEnum::Nullsoft:
        case InstallerTypeEnum::Wix:
            if (doUninstallPrevious)
            {
                context <<
                    GetUninstallInfo <<
                    ExecuteUninstaller;
                context.ClearFlags(Execution::ContextFlag::InstallerExecutionUseUpdate);
            }
            if (ShouldUseDirectMSIInstall(m_installerType, context.Args.Contains(Execution::Args::Type::Silent)))
            {
                context << details::DirectMSIInstall;
            }
            else
            {
                context << details::ShellExecuteInstall;
            }
            break;
        case InstallerTypeEnum::Msix:
            context << details::MsixInstall;
            break;
        case InstallerTypeEnum::MSStore:
            context <<
                EnsureStorePolicySatisfied <<
                (isUpdate ? MSStoreUpdate : MSStoreInstall);
            break;
        case InstallerTypeEnum::Portable:
            if (doUninstallPrevious)
            {
                context <<
                    GetUninstallInfo <<
                    ExecuteUninstaller;
                context.ClearFlags(Execution::ContextFlag::InstallerExecutionUseUpdate);
            }
            context << details::PortableInstall;
            break;
        case InstallerTypeEnum::Zip:
            context << details::ArchiveInstall;
            break;
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }

    void EnsureRunningAsAdminForMachineScopeInstall(Execution::Context& context)
    {
        // Admin is required for machine scope install for installer types like portable, msix and msstore.
        auto installerType = context.Get<Execution::Data::Installer>().value().EffectiveInstallerType();

        if (Manifest::DoesInstallerTypeRequireAdminForMachineScopeInstall(installerType))
        {
            Manifest::ScopeEnum scope = ConvertToScopeEnum(context.Args.GetArg(Execution::Args::Type::InstallScope));
            if (scope == Manifest::ScopeEnum::Machine)
            {
                context << Workflow::EnsureRunningAsAdmin;
            }
        }
    }

    void ExecuteInstaller(Execution::Context& context)
    {
        context << Workflow::ExecuteInstallerForType(context.Get<Execution::Data::Installer>().value().BaseInstallerType);
    }

    void ReportInstallerResult::operator()(Execution::Context& context) const
    {
        DWORD installResult = context.Get<Execution::Data::OperationReturnCode>();
        const auto& additionalSuccessCodes = context.Get<Execution::Data::Installer>()->InstallerSuccessCodes;
        if (installResult != 0 && (std::find(additionalSuccessCodes.begin(), additionalSuccessCodes.end(), installResult) == additionalSuccessCodes.end()))
        {
            const auto& manifest = context.Get<Execution::Data::Manifest>();
            Logging::Telemetry().LogInstallerFailure(manifest.Id, manifest.Version, manifest.Channel, m_installerType, installResult);

            if (m_isHResult)
            {
                context.Reporter.Error()
                    << Resource::String::InstallerFailedWithCode(Utility::LocIndView{ GetUserPresentableMessage(installResult) })
                    << std::endl;
            }
            else
            {
                context.Reporter.Error()
                    << Resource::String::InstallerFailedWithCode(installResult)
                    << std::endl;
            }

            // Show installer log path if exists
            if (context.Contains(Execution::Data::LogPath) && std::filesystem::exists(context.Get<Execution::Data::LogPath>()))
            {
                auto installerLogPath = Utility::LocIndString{ context.Get<Execution::Data::LogPath>().u8string() };
                context.Reporter.Info() << Resource::String::InstallerLogAvailable(installerLogPath) << std::endl;
            }

            // Show a specific message if we can identify the return code
            const auto& expectedReturnCodes = context.Get<Execution::Data::Installer>()->ExpectedReturnCodes;
            auto expectedReturnCodeItr = expectedReturnCodes.find(installResult);
            if (expectedReturnCodeItr != expectedReturnCodes.end() && expectedReturnCodeItr->second.ReturnResponseEnum != ExpectedReturnCodeEnum::Unknown)
            {
                auto returnCode = ExpectedReturnCode::GetExpectedReturnCode(expectedReturnCodeItr->second.ReturnResponseEnum);
                context.Reporter.Error() << returnCode.Message << std::endl;

                auto returnResponseUrl = expectedReturnCodeItr->second.ReturnResponseUrl;
                if (!returnResponseUrl.empty())
                {
                    context.Reporter.Error() << Resource::String::RelatedLink << ' ' << returnResponseUrl << std::endl;
                }

                AICLI_TERMINATE_CONTEXT(returnCode.HResult);
            }

            AICLI_TERMINATE_CONTEXT(m_hr);
        }
        else
        {
            context.Reporter.Info() << Resource::String::InstallFlowInstallSuccess << std::endl;
        }
    }

    void ReportIdentityAndInstallationDisclaimer(Execution::Context& context)
    {
        context <<
            Workflow::ReportManifestIdentityWithVersion() <<
            Workflow::ShowInstallationDisclaimer;
    }

    void InstallPackageInstaller(Execution::Context& context)
    {
        context <<
            Workflow::ReportExecutionStage(ExecutionStage::PreExecution) <<
            Workflow::SnapshotARPEntries <<
            Workflow::ReportExecutionStage(ExecutionStage::Execution) <<
            Workflow::ExecuteInstaller <<
            Workflow::ReportExecutionStage(ExecutionStage::PostExecution) <<
            Workflow::ReportARPChanges <<
            Workflow::RecordInstall <<
            Workflow::RemoveInstaller << 
            Workflow::DisplayInstallationNotes;
    }

    void DownloadSinglePackage(Execution::Context& context)
    {
        // TODO: Split dependencies from download flow to prevent multiple installations.
        context <<
            Workflow::ReportIdentityAndInstallationDisclaimer <<
            Workflow::ShowPromptsForSinglePackage(/* ensureAcceptance */ true) <<
            Workflow::GetDependenciesFromInstaller <<
            Workflow::ReportDependencies(Resource::String::InstallAndUpgradeCommandsReportDependencies) <<
            Workflow::EnableWindowsFeaturesDependencies <<
            Workflow::ManagePackageDependencies(Resource::String::InstallAndUpgradeCommandsReportDependencies) <<
            Workflow::DownloadInstaller;
    }

    void InstallSinglePackage(Execution::Context& context)
    {
        context <<
            Workflow::CheckForUnsupportedArgs <<
            Workflow::DownloadSinglePackage <<
            Workflow::InstallPackageInstaller;
    }

    void EnsureSupportForInstall(Execution::Context& context)
    {
        context <<
            Workflow::EnsureRunningAsAdminForMachineScopeInstall <<
            Workflow::EnsureSupportForPortableInstall <<
            Workflow::EnsureValidNestedInstallerMetadataForArchiveInstall;
    }

    void InstallMultiplePackages::operator()(Execution::Context& context) const
    {
        // Show all prompts needed for every package before installing anything
        context << Workflow::ShowPromptsForMultiplePackages(m_ensurePackageAgreements);

        if (context.IsTerminated())
        {
            return;
        }

        // Report dependencies
        if (Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Dependencies))
        {
            DependencyList allDependencies;
            for (auto& packageContext : context.Get<Execution::Data::PackageSubContexts>())
            {
                allDependencies.Add(packageContext->Get<Execution::Data::Installer>().value().Dependencies);
            }

            context.Add<Execution::Data::Dependencies>(allDependencies);
            context << Workflow::ReportDependencies(m_dependenciesReportMessage);
        }

        bool allSucceeded = true;
        size_t packagesCount = context.Get<Execution::Data::PackageSubContexts>().size();
        size_t packagesProgress = 0;

        for (auto& packageContext : context.Get<Execution::Data::PackageSubContexts>())
        {
            packagesProgress++;
            context.Reporter.Info() << '(' << packagesProgress << '/' << packagesCount << ") "_liv;

            // We want to do best effort to install all packages regardless of previous failures
            Execution::Context& installContext = *packageContext;
            auto previousThreadGlobals = installContext.SetForCurrentThread();

            installContext << Workflow::ReportIdentityAndInstallationDisclaimer;

            // Prevent individual exceptions from breaking out of the loop
            try
            {
                if (!m_ignorePackageDependencies)
                {
                    installContext << Workflow::ManagePackageDependencies(m_dependenciesReportMessage);
                }
                installContext <<
                    Workflow::DownloadInstaller <<
                    Workflow::InstallPackageInstaller;
            }
            catch (...)
            {
                installContext.SetTerminationHR(Workflow::HandleException(installContext, std::current_exception()));
            }

            installContext.Reporter.Info() << std::endl;

            if (installContext.IsTerminated())
            {
                if (context.IsTerminated() && context.GetTerminationHR() == E_ABORT)
                {
                    // This means that the subcontext being terminated is due to an overall abort
                    context.Reporter.Info() << Resource::String::Cancelled << std::endl;
                    return;
                }

                if (m_ignorableInstallResults.end() == std::find(m_ignorableInstallResults.begin(), m_ignorableInstallResults.end(), installContext.GetTerminationHR()))
                {
                    allSucceeded = false;
                    if (m_stopOnFailure)
                    {
                        break;
                    }
                }
            }
        }

        if (!allSucceeded)
        {
            AICLI_TERMINATE_CONTEXT(m_resultOnFailure);
        }
    }

    void SnapshotARPEntries(Execution::Context& context) try
    {
        // Ensure that installer type might actually write to ARP, otherwise this is a waste of time
        auto installer = context.Get<Execution::Data::Installer>();

        if (installer && MightWriteToARP(installer->EffectiveInstallerType()))
        {
            Repository::Correlation::ARPCorrelationData data;
            data.CapturePreInstallSnapshot();
            context.Add<Execution::Data::ARPCorrelationData>(std::move(data));
        }
    }
    CATCH_LOG()

    void ReportARPChanges(Execution::Context& context) try
    {
        if (!context.Contains(Execution::Data::ARPCorrelationData))
        {
            return;
        }

        const auto& manifest = context.Get<Execution::Data::Manifest>();
        auto& arpCorrelationData = context.Get<Execution::Data::ARPCorrelationData>();

        arpCorrelationData.CapturePostInstallSnapshot();
        auto correlationResult = arpCorrelationData.CorrelateForNewlyInstalled(manifest);

        // Store the ARP entry found to match the package to record it in the tracking catalog later
        if (correlationResult.Package)
        {
            std::vector<AppsAndFeaturesEntry> entries;

            auto metadata = correlationResult.Package->GetMetadata();

            AppsAndFeaturesEntry baseEntry;

            // Display name and publisher are also available as multi properties, but
            // for ARP there will always be only 0 or 1 values.
            baseEntry.DisplayName = correlationResult.Package->GetProperty(PackageVersionProperty::Name).get();
            baseEntry.Publisher = correlationResult.Package->GetProperty(PackageVersionProperty::Publisher).get();
            baseEntry.DisplayVersion = correlationResult.Package->GetProperty(PackageVersionProperty::Version).get();
            baseEntry.InstallerType = Manifest::ConvertToInstallerTypeEnum(metadata[PackageVersionMetadata::InstalledType]);

            auto productCodes = correlationResult.Package->GetMultiProperty(PackageVersionMultiProperty::ProductCode);
            for (auto&& productCode : productCodes)
            {
                AppsAndFeaturesEntry entry = baseEntry;
                entry.ProductCode = std::move(productCode).get();
                entries.push_back(std::move(entry));
            }

            auto upgradeCodes = correlationResult.Package->GetMultiProperty(PackageVersionMultiProperty::UpgradeCode);
            for (auto&& upgradeCode : upgradeCodes)
            {
                AppsAndFeaturesEntry entry = baseEntry;
                entry.UpgradeCode= std::move(upgradeCode).get();
                entries.push_back(std::move(entry));
            }

            context.Add<Data::CorrelatedAppsAndFeaturesEntries>(std::move(entries));
        }

        // We can only get the source identifier from an active source
        std::string sourceIdentifier;
        if (context.Contains(Execution::Data::PackageVersion))
        {
            sourceIdentifier = context.Get<Execution::Data::PackageVersion>()->GetProperty(PackageVersionProperty::SourceIdentifier);
        }

        IPackageVersion::Metadata arpEntryMetadata;
        if (correlationResult.Package)
        {
            arpEntryMetadata = correlationResult.Package->GetMetadata();
        }

        Logging::Telemetry().LogSuccessfulInstallARPChange(
            sourceIdentifier,
            manifest.Id,
            manifest.Version,
            manifest.Channel,
            correlationResult.ChangesToARP,
            correlationResult.MatchesInARP,
            correlationResult.CountOfIntersectionOfChangesAndMatches,
            correlationResult.Package ? static_cast<std::string>(correlationResult.Package->GetProperty(PackageVersionProperty::Name)) : "",
            correlationResult.Package ? static_cast<std::string>(correlationResult.Package->GetProperty(PackageVersionProperty::Version)) : "",
            correlationResult.Package ? static_cast<std::string>(correlationResult.Package->GetProperty(PackageVersionProperty::Publisher)) : "",
            correlationResult.Package ? static_cast<std::string_view>(arpEntryMetadata[PackageVersionMetadata::InstalledLocale]) : ""
        );
    }
    CATCH_LOG();

    void RecordInstall(Context& context)
    {
        // Local manifest installs won't have a package version, and tracking them doesn't provide much
        // value currently. If we ever do use our own database as a primary source of packages that we
        // maintain, this decision will probably have to be reconsidered.
        if (!context.Contains(Data::PackageVersion))
        {
            return;
        }

        auto manifest = context.Get<Data::Manifest>();

        // If we have determined an ARP entry matches the installed package,
        // we set its product code in the manifest we record to ensure we can
        // find it in the future.
        // Note that this may overwrite existing information.
        if (context.Contains(Data::CorrelatedAppsAndFeaturesEntries))
        {
            // Use a new Installer entry
            manifest.Installers.emplace_back();
            manifest.Installers.back().AppsAndFeaturesEntries = context.Get<Data::CorrelatedAppsAndFeaturesEntries>();
        }

        auto trackingCatalog = context.Get<Data::PackageVersion>()->GetSource().GetTrackingCatalog();

        auto version = trackingCatalog.RecordInstall(
            manifest,
            context.Get<Data::Installer>().value(),
            WI_IsFlagSet(context.GetFlags(), ContextFlag::InstallerExecutionUseUpdate));

        // Record user intent values. Command args takes precedence. Then previous user intent values.
        Repository::IPackageVersion::Metadata installedMetadata;
        if (context.Contains(Data::InstalledPackageVersion) && context.Get<Execution::Data::InstalledPackageVersion>())
        {
            installedMetadata = context.Get<Data::InstalledPackageVersion>()->GetMetadata();
        }

        if (context.Args.Contains(Execution::Args::Type::InstallArchitecture))
        {
            version.SetMetadata(Repository::PackageVersionMetadata::UserIntentArchitecture, context.Args.GetArg(Execution::Args::Type::InstallArchitecture));
        }
        else
        {
            auto itr = installedMetadata.find(Repository::PackageVersionMetadata::UserIntentArchitecture);
            if (itr != installedMetadata.end())
            {
                version.SetMetadata(Repository::PackageVersionMetadata::UserIntentArchitecture, itr->second);
            }
        }

        if (context.Args.Contains(Execution::Args::Type::Locale))
        {
            version.SetMetadata(Repository::PackageVersionMetadata::UserIntentLocale, context.Args.GetArg(Execution::Args::Type::Locale));
        }
        else
        {
            auto itr = installedMetadata.find(Repository::PackageVersionMetadata::UserIntentLocale);
            if (itr != installedMetadata.end())
            {
                version.SetMetadata(Repository::PackageVersionMetadata::UserIntentLocale, itr->second);
            }
        }
    }
}
