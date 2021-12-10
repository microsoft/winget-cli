// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallFlow.h"
#include "DownloadFlow.h"
#include "UninstallFlow.h"
#include "ShowFlow.h"
#include "Resources.h"
#include "ShellExecuteInstallerHandler.h"
#include "MSStoreInstallerHandler.h"
#include "MsiInstallFlow.h"
#include "WorkflowBase.h"
#include "Workflows/DependenciesFlow.h"
#include <AppInstallerDeployment.h>

using namespace winrt::Windows::ApplicationModel::Store::Preview::InstallControl;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Management::Deployment;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Utility;


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
            context.Reporter.Info() << Execution::PromptEmphasis << Resource::String::InstallationDisclaimerMSStore << std::endl;
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
        for (auto& packageContext : context.Get<Execution::Data::PackagesToInstall>())
        {
            // Show agreements for each package
            Execution::Context& showContext = *packageContext;
            auto previousThreadGlobals = showContext.SetForCurrentThread();

            showContext <<
                Workflow::ReportManifestIdentityWithVersion <<
                Workflow::ShowPackageAgreements(/* ensureAcceptance */ false);
            if (showContext.IsTerminated())
            {
                AICLI_TERMINATE_CONTEXT(showContext.GetTerminationHR());
            }

            hasPackageAgreements |= !showContext.Get<Execution::Data::Manifest>().CurrentLocalization.Get<AppInstaller::Manifest::Localization::Agreements>().empty();
        }

        // If any package has agreements, ensure they are accepted
        if (hasPackageAgreements)
        {
            context << Workflow::EnsurePackageAgreementsAcceptance(/* showPrompt */ false);
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
            ShellExecuteInstallImpl <<
            ReportInstallerResult("ShellExecute"sv, APPINSTALLER_CLI_ERROR_SHELLEXEC_INSTALL_FAILED);
    }

    void DirectMSIInstall(Execution::Context& context)
    {
        context <<
            GetInstallerArgs <<
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

    void ReportIdentityAndInstallationDisclaimer(Execution::Context& context)
    {
        context <<
            Workflow::ReportManifestIdentityWithVersion <<
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
            Workflow::RemoveInstaller;
    }

    void DownloadSinglePackage(Execution::Context& context)
    {
        context <<
            Workflow::ReportIdentityAndInstallationDisclaimer <<
            Workflow::ShowPackageAgreements(/* ensureAcceptance */ true) <<
            Workflow::GetDependenciesFromInstaller <<
            Workflow::ReportDependencies(Resource::String::InstallAndUpgradeCommandsReportDependencies) <<
            Workflow::ManagePackageDependencies(Resource::String::InstallAndUpgradeCommandsReportDependencies) <<
            Workflow::DownloadInstaller;
    }

    void InstallSinglePackage(Execution::Context& context)
    {
        context <<
            Workflow::DownloadSinglePackage <<
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
            for (auto& packageContext : context.Get<Execution::Data::PackagesToInstall>())
            {
                allDependencies.Add(packageContext->Get<Execution::Data::Installer>().value().Dependencies);
            }

            context.Add<Execution::Data::Dependencies>(allDependencies);
            context << Workflow::ReportDependencies(m_dependenciesReportMessage);
        }

        bool allSucceeded = true;
        size_t packagesCount = context.Get<Execution::Data::PackagesToInstall>().size();
        size_t packagesProgress = 0;
        
        for (auto& packageContext : context.Get<Execution::Data::PackagesToInstall>())
        {
            packagesProgress++;
            context.Reporter.Info() << "(" << packagesProgress << "/" << packagesCount << ") ";

            // We want to do best effort to install all packages regardless of previous failures
            Execution::Context& installContext = *packageContext;
            auto previousThreadGlobals = installContext.SetForCurrentThread();

            installContext << Workflow::ReportIdentityAndInstallationDisclaimer;
            if (!m_ignorePackageDependencies)
            {
                installContext << Workflow::ManagePackageDependencies(m_dependenciesReportMessage);
            }
            installContext << Workflow::DownloadInstaller;
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
            Source arpSource = context.Reporter.ExecuteWithProgress(
                [](IProgressCallback& progress)
                {
                    Repository::Source result = Repository::Source(PredefinedSource::ARP);
                    result.Open(progress);
                    return result;
                }, true);

            std::vector<std::tuple<Utility::LocIndString, Utility::LocIndString, Utility::LocIndString>> entries;

            for (const auto& entry : arpSource.Search({}).Matches)
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
            Source arpSource = context.Reporter.ExecuteWithProgress(
                [](IProgressCallback& progress)
                {
                    Repository::Source result = Repository::Source(PredefinedSource::ARP);
                    result.Open(progress);
                    return result;
                }, true);

            std::vector<ResultMatch> changes;

            for (auto& entry : arpSource.Search({}).Matches)
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
                findByManifest = arpSource.Search(nameAndPublisherRequest);
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

            // Find the package that we are going to log
            std::shared_ptr<IPackageVersion> toLog;

            // If there is only a single common package (changed and matches), it is almost certainly the correct one.
            if (packagesInBoth.size() == 1)
            {
                toLog = packagesInBoth[0]->GetInstalledVersion();
            }
            // If it wasn't changed but we still find a match, that is the best thing to report.
            else if (findByManifest.Matches.size() == 1)
            {
                toLog = findByManifest.Matches[0].Package->GetInstalledVersion();
            }
            // If only a single ARP entry was changed and we found no matches, report that.
            else if (findByManifest.Matches.empty() && changes.size() == 1)
            {
                toLog = changes[0].Package->GetInstalledVersion();
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

        auto trackingCatalog = context.Get<Data::PackageVersion>()->GetSource().GetTrackingCatalog();

        trackingCatalog.RecordInstall(
            context.Get<Data::Manifest>(),
            context.Get<Data::Installer>().value(),
            WI_IsFlagSet(context.GetFlags(), ContextFlag::InstallerExecutionUseUpdate));
    }
}
