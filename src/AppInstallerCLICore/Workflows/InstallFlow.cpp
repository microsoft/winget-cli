// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallFlow.h"
#include "UninstallFlow.h"
#include "ShowFlow.h"
#include "Resources.h"
#include "ShellExecuteInstallerHandler.h"
#include "MSStoreInstallerHandler.h"
#include "MsiInstallFlow.h"
#include "WorkflowBase.h"
#include "DependenciesFlow.h"

#include <AppInstallerDeployment.h>
#include <AppInstallerMsixInfo.h>

namespace AppInstaller::CLI::Workflow
{
    using namespace winrt::Windows::ApplicationModel::Store::Preview::InstallControl;
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Windows::Foundation::Collections;
    using namespace winrt::Windows::Management::Deployment;
    using namespace AppInstaller::Utility;
    using namespace AppInstaller::Manifest;
    using namespace AppInstaller::Repository;
    using namespace AppInstaller::Settings;

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
                default:
                    THROW_HR(E_UNEXPECTED);
                }
            }

            ExpectedReturnCodeEnum InstallerReturnCode;
            HRESULT HResult;
            Resource::StringId Message;
        };
    }

    void EnsureApplicableInstaller(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>();

        if (!installer.has_value())
        {
            context.Reporter.Error() << Resource::String::NoApplicableInstallers << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER);
        }
    }

    void ShowInstallationDisclaimer(Execution::Context& context)
    {
        auto installerType = context.Get<Execution::Data::Installer>().value().InstallerType;

        if (installerType == InstallerTypeEnum::MSStore)
        {
            context.Reporter.Info() << Resource::String::InstallationDisclaimerMSStore << std::endl;
        }
        else
        {
            context.Reporter.Info() <<
                Resource::String::InstallationDisclaimer1 << std::endl <<
                Resource::String::InstallationDisclaimer2 << std::endl;
        }
    }

    void ShowPackageAgreements::operator()(Execution::Context& context) const
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        auto agreements = manifest.CurrentLocalization.Get<AppInstaller::Manifest::Localization::Agreements>();

        if (agreements.empty())
        {
            // Nothing to do
            return;
        }

        context << Workflow::ShowPackageInfo;
        context.Reporter.Info() << std::endl;

        if (m_ensureAcceptance)
        {
            context << Workflow::EnsurePackageAgreementsAcceptance(/* showPrompt */ true);
        }
    }

    void EnsurePackageAgreementsAcceptance::operator()(Execution::Context& context) const
    {
        if (WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::AgreementsAcceptedByCaller))
        {
            AICLI_LOG(CLI, Info, << "Skipping package agreements acceptance check because AgreementsAcceptedByCaller flag is set.");
            return;
        }

        if (context.Args.Contains(Execution::Args::Type::AcceptPackageAgreements))
        {
            AICLI_LOG(CLI, Info, << "Package agreements accepted by CLI flag");
            return;
        }

        if (m_showPrompt)
        {
            bool accepted = context.Reporter.PromptForBoolResponse(Resource::String::PackageAgreementsPrompt);
            if (accepted)
            {
                AICLI_LOG(CLI, Info, << "Package agreements accepted in prompt");
                return;
            }
            else
            {
                AICLI_LOG(CLI, Info, << "Package agreements not accepted in prompt");
            }
        }

        AICLI_LOG(CLI, Error, << "Package agreements were not agreed to.");
        context.Reporter.Error() << Resource::String::PackageAgreementsNotAgreedTo << std::endl;
        AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PACKAGE_AGREEMENTS_NOT_ACCEPTED);
    }

    void EnsurePackageAgreementsAcceptanceForMultipleInstallers(Execution::Context& context)
    {
        bool hasPackageAgreements = false;
        for (auto package : context.Get<Execution::Data::PackagesToInstall>())
        {
            // Show agreements for each package in a sub-context
            auto showContextPtr = context.Clone();
            Execution::Context& showContext = *showContextPtr;

            showContext.Add<Execution::Data::Manifest>(package.Manifest);

            showContext <<
                Workflow::ReportManifestIdentityWithVersion <<
                Workflow::ShowPackageAgreements(/* ensureAcceptance */ false);
            if (showContext.IsTerminated())
            {
                AICLI_TERMINATE_CONTEXT(showContext.GetTerminationHR());
            }

            hasPackageAgreements |= !package.Manifest.CurrentLocalization.Get<AppInstaller::Manifest::Localization::Agreements>().empty();
        }

        // If any package has agreements, ensure they are accepted
        if (hasPackageAgreements)
        {
            context << Workflow::EnsurePackageAgreementsAcceptance(/* showPrompt */ false);
        }
    }

    void DownloadInstaller(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>().value();

        switch (installer.InstallerType)
        {
        case InstallerTypeEnum::Exe:
        case InstallerTypeEnum::Burn:
        case InstallerTypeEnum::Inno:
        case InstallerTypeEnum::Msi:
        case InstallerTypeEnum::Nullsoft:
        case InstallerTypeEnum::Wix:
            context << DownloadInstallerFile << VerifyInstallerHash << UpdateInstallerFileMotwIfApplicable;
            break;
        case InstallerTypeEnum::Msix:
            if (installer.SignatureSha256.empty())
            {
                context << DownloadInstallerFile << VerifyInstallerHash << UpdateInstallerFileMotwIfApplicable;
            }
            else
            {
                // Signature hash provided. No download needed. Just verify signature hash.
                context << GetMsixSignatureHash << VerifyInstallerHash << UpdateInstallerFileMotwIfApplicable;
            }
            break;
        case InstallerTypeEnum::MSStore:
            // Nothing to do here
            break;
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }

    void DownloadInstallerFile(Execution::Context& context)
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        const auto& installer = context.Get<Execution::Data::Installer>().value();

        std::filesystem::path tempInstallerPath = Runtime::GetPathTo(Runtime::PathName::Temp);
        tempInstallerPath /= Utility::ConvertToUTF16(manifest.Id + '.' + manifest.Version);

        Utility::DownloadInfo downloadInfo{};
        downloadInfo.DisplayName = Resource::GetFixedString(Resource::FixedString::ProductName);
        // Use the SHA256 hash of the installer as the identifier for the download
        downloadInfo.ContentId = SHA256::ConvertToString(installer.Sha256);

        AICLI_LOG(CLI, Info, << "Generated temp download path: " << tempInstallerPath);

        context.Reporter.Info() << "Downloading " << Execution::UrlEmphasis << installer.Url << std::endl;

        std::optional<std::vector<BYTE>> hash;

        const int MaxRetryCount = 2;
        for (int retryCount = 0; retryCount < MaxRetryCount; ++retryCount)
        {
            bool success = false;
            try
            {
                hash = context.Reporter.ExecuteWithProgress(std::bind(Utility::Download,
                    installer.Url,
                    tempInstallerPath,
                    Utility::DownloadType::Installer,
                    std::placeholders::_1,
                    true,
                    downloadInfo));

                success = true;
            }
            catch (...)
            {
                if (retryCount < MaxRetryCount - 1)
                {
                    AICLI_LOG(CLI, Info, << "Failed to download, waiting a bit and retry. Url: " << installer.Url);
                    Sleep(500);
                }
                else
                {
                    throw;
                }
            }

            if (success)
            {
                break;
            }
        }

        if (!hash)
        {
            context.Reporter.Info() << "Package download canceled." << std::endl;
            AICLI_TERMINATE_CONTEXT(E_ABORT);
        }

        context.Add<Execution::Data::HashPair>(std::make_pair(installer.Sha256, hash.value()));
        context.Add<Execution::Data::InstallerPath>(std::move(tempInstallerPath));
    }

    void GetMsixSignatureHash(Execution::Context& context)
    {
        // We use this when the server won't support streaming install to swap to download.
        bool downloadInstead = false;

        try
        {
            const auto& installer = context.Get<Execution::Data::Installer>().value();

            Msix::MsixInfo msixInfo(installer.Url);
            auto signature = msixInfo.GetSignature();

            auto signatureHash = SHA256::ComputeHash(signature.data(), static_cast<uint32_t>(signature.size()));

            context.Add<Execution::Data::HashPair>(std::make_pair(installer.SignatureSha256, signatureHash));
        }
        catch (const winrt::hresult_error& e)
        {
            if (static_cast<HRESULT>(e.code()) == HRESULT_FROM_WIN32(ERROR_NO_RANGES_PROCESSED) ||
                HRESULT_FACILITY(e.code()) == FACILITY_HTTP)
            {
                // Failed to get signature hash through HttpStream, use download
                downloadInstead = true;
            }
            else
            {
                throw;
            }
        }

        if (downloadInstead)
        {
            context << DownloadInstallerFile;
        }
    }

    void VerifyInstallerHash(Execution::Context& context)
    {
        const auto& hashPair = context.Get<Execution::Data::HashPair>();

        if (!std::equal(
            hashPair.first.begin(),
            hashPair.first.end(),
            hashPair.second.begin()))
        {
            bool overrideHashMismatch = context.Args.Contains(Execution::Args::Type::HashOverride);

            const auto& manifest = context.Get<Execution::Data::Manifest>();
            Logging::Telemetry().LogInstallerHashMismatch(manifest.Id, manifest.Version, manifest.Channel, hashPair.first, hashPair.second, overrideHashMismatch);

            // If running as admin, do not allow the user to override the hash failure.
            if (Runtime::IsRunningAsAdmin())
            {
                context.Reporter.Error() << Resource::String::InstallerHashMismatchAdminBlock << std::endl;
            }
            else if (overrideHashMismatch)
            {
                context.Reporter.Warn() << Resource::String::InstallerHashMismatchOverridden << std::endl;
                return;
            }
            else if (Settings::GroupPolicies().IsEnabled(Settings::TogglePolicy::Policy::HashOverride))
            {
                context.Reporter.Error() << Resource::String::InstallerHashMismatchOverrideRequired << std::endl;
            }
            else
            {
                context.Reporter.Error() << Resource::String::InstallerHashMismatchError << std::endl;
            }

            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INSTALLER_HASH_MISMATCH);
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Installer hash verified");
            context.Reporter.Info() << Resource::String::InstallerHashVerified << std::endl;

            context.SetFlags(Execution::ContextFlag::InstallerHashMatched);

            if (context.Contains(Execution::Data::PackageVersion) &&
                context.Get<Execution::Data::PackageVersion>()->GetSource() != nullptr &&
                WI_IsFlagSet(context.Get<Execution::Data::PackageVersion>()->GetSource()->GetDetails().TrustLevel, SourceTrustLevel::Trusted))
            {
                context.SetFlags(Execution::ContextFlag::InstallerTrusted);
            }
        }
    }

    void UpdateInstallerFileMotwIfApplicable(Execution::Context& context)
    {
        if (context.Contains(Execution::Data::InstallerPath))
        {
            if (WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerTrusted))
            {
                Utility::ApplyMotwIfApplicable(context.Get<Execution::Data::InstallerPath>(), URLZONE_TRUSTED);
            }
            else if (WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerHashMatched))
            {
                const auto& installer = context.Get<Execution::Data::Installer>();
                HRESULT hr = Utility::ApplyMotwUsingIAttachmentExecuteIfApplicable(context.Get<Execution::Data::InstallerPath>(), installer.value().Url, URLZONE_INTERNET);

                // Not using SUCCEEDED(hr) to check since there are cases file is missing after a successful scan
                if (hr != S_OK)
                {
                    switch (hr)
                    {
                    case INET_E_SECURITY_PROBLEM:
                        context.Reporter.Error() << Resource::String::InstallerBlockedByPolicy << std::endl;
                        break;
                    case E_FAIL:
                        context.Reporter.Error() << Resource::String::InstallerFailedVirusScan << std::endl;
                        break;
                    default:
                        context.Reporter.Error() << Resource::String::InstallerFailedSecurityCheck << std::endl;
                    }

                    AICLI_LOG(Fail, Error, << "Installer failed security check. Url: " << installer.value().Url << " Result: " << WINGET_OSTREAM_FORMAT_HRESULT(hr));
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INSTALLER_SECURITY_CHECK_FAILED);
                }
            }
        }
    }

    void ExecuteInstaller(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>().value();

        bool isUpdate = WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerExecutionUseUpdate);

        switch (installer.InstallerType)
        {
        case InstallerTypeEnum::Exe:
        case InstallerTypeEnum::Burn:
        case InstallerTypeEnum::Inno:
        case InstallerTypeEnum::Msi:
        case InstallerTypeEnum::Nullsoft:
        case InstallerTypeEnum::Wix:
            if (isUpdate && installer.UpdateBehavior == UpdateBehaviorEnum::UninstallPrevious)
            {
                context <<
                    GetUninstallInfo <<
                    ExecuteUninstaller;
                context.ClearFlags(Execution::ContextFlag::InstallerExecutionUseUpdate);
            }
            if (ShouldUseDirectMSIInstall(installer.InstallerType, context.Args.Contains(Execution::Args::Type::Silent)))
            {
                context << DirectMSIInstall;
            }
            else
            {
                context << ShellExecuteInstall;
            }
            break;
        case InstallerTypeEnum::Msix:
            context << MsixInstall;
            break;
        case InstallerTypeEnum::MSStore:
            context <<
                EnsureStorePolicySatisfied <<
                (isUpdate ? MSStoreUpdate : MSStoreInstall);
            break;
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }

    void ShellExecuteInstall(Execution::Context& context)
    {
        context <<
            GetInstallerArgs <<
            RenameDownloadedInstaller <<
            ShellExecuteInstallImpl <<
            ReportInstallerResult("ShellExecute"sv, APPINSTALLER_CLI_ERROR_SHELLEXEC_INSTALL_FAILED);
    }

    void DirectMSIInstall(Execution::Context& context)
    {
        context <<
            GetInstallerArgs <<
            RenameDownloadedInstaller <<
            DirectMSIInstallImpl <<
            ReportInstallerResult("MsiInstallProduct"sv, APPINSTALLER_CLI_ERROR_MSI_INSTALL_FAILED);
    }

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

        context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

        bool registrationDeferred = false;

        try
        {
            registrationDeferred = context.Reporter.ExecuteWithProgress([&](IProgressCallback& callback)
            {
                return Deployment::AddPackageWithDeferredFallback(uri, WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerTrusted), callback);
            });
        }
        catch (const wil::ResultException& re)
        {
            context.Add<Execution::Data::InstallerReturnCode>(re.GetErrorCode());
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

    void ReportInstallerResult::operator()(Execution::Context& context) const
    {
        DWORD installResult = context.Get<Execution::Data::InstallerReturnCode>();
        const auto& additionalSuccessCodes = context.Get<Execution::Data::Installer>()->InstallerSuccessCodes;
        if (installResult != 0 && (std::find(additionalSuccessCodes.begin(), additionalSuccessCodes.end(), installResult) == additionalSuccessCodes.end()))
        {
            const auto& manifest = context.Get<Execution::Data::Manifest>();
            Logging::Telemetry().LogInstallerFailure(manifest.Id, manifest.Version, manifest.Channel, m_installerType, installResult);

            if (m_isHResult)
            {
                context.Reporter.Error() << Resource::String::InstallerFailedWithCode << ' ' << GetUserPresentableMessage(installResult) << std::endl;
            }
            else
            {
                context.Reporter.Error() << Resource::String::InstallerFailedWithCode << ' ' << installResult << std::endl;
            }

            // Show installer log path if exists
            if (context.Contains(Execution::Data::LogPath) && std::filesystem::exists(context.Get<Execution::Data::LogPath>()))
            {
                context.Reporter.Info() << Resource::String::InstallerLogAvailable << ' ' << context.Get<Execution::Data::LogPath>().u8string() << std::endl;
            }

            // Show a specific message if we can identify the return code
            const auto& expectedReturnCodes = context.Get<Execution::Data::Installer>()->ExpectedReturnCodes;
            auto expectedReturnCodeItr = expectedReturnCodes.find(installResult);
            if (expectedReturnCodeItr != expectedReturnCodes.end() && expectedReturnCodeItr->second != ExpectedReturnCodeEnum::Unknown)
            {
                auto returnCode = ExpectedReturnCode::GetExpectedReturnCode(expectedReturnCodeItr->second);
                context.Reporter.Error() << returnCode.Message << std::endl;
                AICLI_TERMINATE_CONTEXT(returnCode.HResult);
            }

            AICLI_TERMINATE_CONTEXT(m_hr);
        }
        else
        {
            context.Reporter.Info() << Resource::String::InstallFlowInstallSuccess << std::endl;
        }
    }

    void RemoveInstaller(Execution::Context& context)
    {
        // Path may not be present if installed from a URL for MSIX
        if (context.Contains(Execution::Data::InstallerPath))
        {
            const auto& path = context.Get<Execution::Data::InstallerPath>();
            AICLI_LOG(CLI, Info, << "Removing installer: " << path);

            try
            {
                // best effort
                std::filesystem::remove(path);
            }
            catch (const std::exception& e)
            {
                AICLI_LOG(CLI, Warning, << "Failed to remove installer file after execution. Reason: " << e.what());
            }
            catch (...)
            {
                AICLI_LOG(CLI, Warning, << "Failed to remove installer file after execution. Reason unknown.");
            }
        }
    }

    void ReportIdentityAndInstallationDisclaimer(Execution::Context& context)
    {
        context <<
            Workflow::ReportManifestIdentityWithVersion <<
            Workflow::ShowInstallationDisclaimer;
    }

    void InstallPackageInstaller(Execution::Context& context)
    {
        context <<
            Workflow::ReportExecutionStage(ExecutionStage::Download) <<
            Workflow::DownloadInstaller <<
            Workflow::ReportExecutionStage(ExecutionStage::PreExecution) <<
            Workflow::SnapshotARPEntries <<
            Workflow::ReportExecutionStage(ExecutionStage::Execution) <<
            Workflow::ExecuteInstaller <<
            Workflow::ReportExecutionStage(ExecutionStage::PostExecution) <<
            Workflow::ReportARPChanges <<
            Workflow::RemoveInstaller;
    }

    void InstallSinglePackage(Execution::Context& context)
    {
        context <<
            Workflow::ReportIdentityAndInstallationDisclaimer <<
            Workflow::ShowPackageAgreements(/* ensureAcceptance */ true) <<
            Workflow::GetDependenciesFromInstaller << 
            Workflow::ReportDependencies(Resource::String::InstallAndUpgradeCommandsReportDependencies) <<
            Workflow::ManagePackageDependencies(Resource::String::InstallAndUpgradeCommandsReportDependencies) <<
            Workflow::InstallPackageInstaller;
    }

    void InstallMultiplePackages::operator()(Execution::Context& context) const
    {
        if (m_ensurePackageAgreements)
        {
            // Show all license agreements before installing anything
            context << Workflow::EnsurePackageAgreementsAcceptanceForMultipleInstallers;
        }

        if (context.IsTerminated())
        {
            return;
        }

        // Report dependencies
        if (Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Dependencies))
        {
            DependencyList allDependencies;
            for (auto package : context.Get<Execution::Data::PackagesToInstall>())
            {
                allDependencies.Add(package.Installer.Dependencies);
            }

            context.Add<Execution::Data::Dependencies>(allDependencies);
            context << Workflow::ReportDependencies(m_dependenciesReportMessage);
        }

        bool allSucceeded = true;
        for (auto package : context.Get<Execution::Data::PackagesToInstall>())
        {
            Logging::SubExecutionTelemetryScope subExecution{ package.PackageSubExecutionId };

            // We want to do best effort to install all packages regardless of previous failures
            auto installContextPtr = context.Clone();
            Execution::Context& installContext = *installContextPtr;

            // Extract the data needed for installing
            installContext.Add<Execution::Data::PackageVersion>(package.PackageVersion);
            installContext.Add<Execution::Data::Manifest>(package.Manifest);
            installContext.Add<Execution::Data::InstalledPackageVersion>(package.InstalledPackageVersion);
            installContext.Add<Execution::Data::Installer>(package.Installer);

            installContext << Workflow::ReportIdentityAndInstallationDisclaimer;
            if (!m_ignorePackageDependencies)
            {
                installContext << Workflow::ManagePackageDependencies(m_dependenciesReportMessage);
            }
            installContext << Workflow::InstallPackageInstaller;
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

        if (installer && MightWriteToARP(installer->InstallerType))
        {
            std::shared_ptr<ISource> arpSource = context.Reporter.ExecuteWithProgress(
                [](IProgressCallback& progress)
                {
                    return Repository::OpenPredefinedSource(PredefinedSource::ARP, progress);
                }, true);

            std::vector<std::tuple<Utility::LocIndString, Utility::LocIndString, Utility::LocIndString>> entries;

            for (const auto& entry : arpSource->Search({}).Matches)
            {
                auto installed = entry.Package->GetInstalledVersion();
                if (installed)
                {
                    entries.emplace_back(std::make_tuple(
                        entry.Package->GetProperty(PackageProperty::Id),
                        installed->GetProperty(PackageVersionProperty::Version),
                        installed->GetProperty(PackageVersionProperty::Channel)));
                }
            }

            std::sort(entries.begin(), entries.end());

            context.Add<Execution::Data::ARPSnapshot>(std::move(entries));
        }
    }
    CATCH_LOG()

    void ReportARPChanges(Execution::Context& context) try
    {
        if (context.Contains(Execution::Data::ARPSnapshot))
        {
            const auto& entries = context.Get<Execution::Data::ARPSnapshot>();

            // Open it again to get the (potentially) changed ARP entries
            std::shared_ptr<ISource> arpSource = context.Reporter.ExecuteWithProgress(
                [](IProgressCallback& progress)
                {
                    return Repository::OpenPredefinedSource(PredefinedSource::ARP, progress);
                }, true);

            std::vector<ResultMatch> changes;

            for (auto& entry : arpSource->Search({}).Matches)
            {
                auto installed = entry.Package->GetInstalledVersion();

                if (installed)
                {
                    auto entryKey = std::make_tuple(
                        entry.Package->GetProperty(PackageProperty::Id),
                        installed->GetProperty(PackageVersionProperty::Version),
                        installed->GetProperty(PackageVersionProperty::Channel));

                    auto itr = std::lower_bound(entries.begin(), entries.end(), entryKey);
                    if (itr == entries.end() || *itr != entryKey)
                    {
                        changes.emplace_back(std::move(entry));
                    }
                }
            }

            // Also attempt to find the entry based on the manifest data
            const auto& manifest = context.Get<Execution::Data::Manifest>();

            SearchRequest nameAndPublisherRequest;

            // The default localization must contain the name or we cannot do this lookup
            if (manifest.DefaultLocalization.Contains(Localization::PackageName))
            {
                AppInstaller::Manifest::Manifest::string_t defaultName = manifest.DefaultLocalization.Get<Localization::PackageName>();
                AppInstaller::Manifest::Manifest::string_t defaultPublisher;
                if (manifest.DefaultLocalization.Contains(Localization::Publisher))
                {
                    defaultPublisher = manifest.DefaultLocalization.Get<Localization::Publisher>();
                }

                nameAndPublisherRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::NormalizedNameAndPublisher, MatchType::Exact, defaultName, defaultPublisher));

                for (const auto& loc : manifest.Localizations)
                {
                    if (loc.Contains(Localization::PackageName) || loc.Contains(Localization::Publisher))
                    {
                        nameAndPublisherRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::NormalizedNameAndPublisher, MatchType::Exact,
                            loc.Contains(Localization::PackageName) ? loc.Get<Localization::PackageName>() : defaultName,
                            loc.Contains(Localization::Publisher) ? loc.Get<Localization::Publisher>() : defaultPublisher));
                    }
                }
            }

            std::vector<std::string> productCodes;
            for (const auto& installer : manifest.Installers)
            {
                if (!installer.ProductCode.empty())
                {
                    if (std::find(productCodes.begin(), productCodes.end(), installer.ProductCode) == productCodes.end())
                    {
                        nameAndPublisherRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::ProductCode, MatchType::Exact, installer.ProductCode));
                        productCodes.emplace_back(installer.ProductCode);
                    }
                }
            }

            SearchResult findByManifest;

            // Don't execute this search if it would just find everything
            if (!nameAndPublisherRequest.IsForEverything())
            {
                findByManifest = arpSource->Search(nameAndPublisherRequest);
            }

            // Cross reference the changes with the search results
            std::vector<std::shared_ptr<IPackage>> packagesInBoth;

            for (const auto& change : changes)
            {
                for (const auto& byManifest : findByManifest.Matches)
                {
                    if (change.Package->IsSame(byManifest.Package.get()))
                    {
                        packagesInBoth.emplace_back(change.Package);
                        break;
                    }
                }
            }

            // We now have all of the package changes; time to report them.
            // The set of cases we could have for changes to ARP:
            //  0 packages  ::  No changes were detected to ARP, which could mean that the installer
            //                  did not write an entry. It could also be a forced reinstall.
            //  1 package   ::  Golden path; this should be what we installed.
            //  2+ packages ::  We need to determine which package actually matches the one that we
            //                  were installing.
            //
            // The set of cases we could have for finding packages based on the manifest:
            //  0 packages  ::  The manifest data does not match the ARP information.
            //  1 package   ::  Golden path; this should be what we installed.
            //  2+ packages ::  The data in the manifest is either too broad or we have
            //                  a problem with our name normalization.
            //
            //                           ARP Package changes
            //                0                  1                     N
            //      +------------------+--------------------+--------------------+
            // M    |                  |                    |                    |
            // a    | Package does not | Manifest data does | Manifest data does |
            // n  0 |   write to ARP   |   not match ARP    |   not match ARP    |
            // i    |  Log this fact   |   Log for fixup    |   Log for fixup    |
            // f    |                  |                    |                    |
            // e    +------------------+--------------------+--------------------+
            // s    |                  |                    |                    |
            // t    |   Reinstall of   |    Golden Path!    | Treat manifest as  |
            //    1 | existing version |  (assuming match)  |   main if common   |
            // r    |                  |                    |                    |
            // e    +------------------+--------------------+--------------------+
            // s    |                  |                    |                    |
            // u    |   Not expected   | Treat ARP as main  |    Not expected    |
            // l  N |   Log this for   |                    |    Log this for    |
            // t    |   investigation  |                    |    investigation   |
            // s    |                  |                    |                    |
            //      +------------------+--------------------+--------------------+

            // Find the package that we are going to log
            std::shared_ptr<IPackageVersion> toLog;

            // If no changes found, only log if a single matching package was found by the manifest
            if (changes.empty() && findByManifest.Matches.size() == 1)
            {
                toLog = findByManifest.Matches[0].Package->GetInstalledVersion();
            }
            // If only a single ARP entry was changed, always log that
            else if (changes.size() == 1)
            {
                toLog = changes[0].Package->GetInstalledVersion();
            }
            // Finally, if there is only a single common package, log that one
            else if (packagesInBoth.size() == 1)
            {
                toLog = packagesInBoth[0]->GetInstalledVersion();
            }

            IPackageVersion::Metadata toLogMetadata;
            if (toLog)
            {
                toLogMetadata = toLog->GetMetadata();
            }

            // We can only get the source identifier from an active source
            std::string sourceIdentifier;
            if (context.Contains(Execution::Data::PackageVersion))
            {
                sourceIdentifier = context.Get<Execution::Data::PackageVersion>()->GetProperty(PackageVersionProperty::SourceIdentifier);
            }

            Logging::Telemetry().LogSuccessfulInstallARPChange(
                sourceIdentifier,
                manifest.Id,
                manifest.Version,
                manifest.Channel,
                changes.size(),
                findByManifest.Matches.size(),
                packagesInBoth.size(),
                toLog ? static_cast<std::string>(toLog->GetProperty(PackageVersionProperty::Name)) : "",
                toLog ? static_cast<std::string>(toLog->GetProperty(PackageVersionProperty::Version)) : "",
                toLog ? static_cast<std::string_view>(toLogMetadata[PackageVersionMetadata::Publisher]) : "",
                toLog ? static_cast<std::string_view>(toLogMetadata[PackageVersionMetadata::InstalledLocale]) : ""
            );
        }
    }
    CATCH_LOG()
}
