// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerCLICore.h"
#include "Microsoft/PredefinedInstalledSourceFactory.h"
#include "Commands/RootCommand.h"
#include "ComContext.h"
#include "ExecutionContext.h"
#include "Workflows/WorkflowBase.h"
#include <winget/UserSettings.h>
#include <winget/Manifest.h>
#include "Commands/COMInstallCommand.h"
#include <AppInstallerTelemetry.h>
#include <AppInstallerErrors.h>
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 6388 Allow CreateInstance.
#include <wil\cppwinrt_wrl.h>
// 4467 Allow use of uuid attribute for com object creation.
#include "PackageManager.h"
#pragma warning( pop )
#include "PackageManager.g.cpp"
#include "InstallResult.h"
#include "PackageCatalogInfo.h"
#include "PackageCatalogReference.h"
#include "PackageVersionInfo.h"
#include "PackageVersionId.h"
#include "Workflows/WorkflowBase.h"
#include "Converters.h"
#include "Helpers.h"
#include "ContextOrchestrator.h"

using namespace std::literals::chrono_literals;
using namespace ::AppInstaller::CLI;

const GUID PackageManagerCLSID1 = { 0xC53A4F16, 0x787E, 0x42A4, { 0xB3, 0x04, 0x29, 0xEF, 0xFB, 0x4B, 0xF5, 0x97 } };  //C53A4F16-787E-42A4-B304-29EFFB4BF597
const GUID PackageManagerCLSID2 = { 0xE65C7D5A, 0x95AF, 0x4A98, { 0xBE, 0x5F, 0xA7, 0x93, 0x02, 0x9C, 0xEB, 0x56 } };  //E65C7D5A-95AF-4A98-BE5F-A793029CEB56

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::PackageCatalogReference> PackageManager::GetPackageCatalogs()
    {
        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::PackageCatalogReference> catalogs{ winrt::single_threaded_vector<Microsoft::Management::Deployment::PackageCatalogReference>() };
        std::vector<::AppInstaller::Repository::SourceDetails> sources = ::AppInstaller::Repository::GetSources();
        for (uint32_t i = 0; i < sources.size(); i++)
        {
            auto packageCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>>();
            packageCatalogInfo->Initialize(sources.at(i));
            auto packageCatalogRef = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference>>();
            packageCatalogRef->Initialize(*packageCatalogInfo);
            catalogs.Append(*packageCatalogRef);
        }
        return catalogs.GetView();
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageManager::GetPredefinedPackageCatalog(winrt::Microsoft::Management::Deployment::PredefinedPackageCatalog const& predefinedPackageCatalog)
    {
        ::AppInstaller::Repository::SourceDetails sourceDetails;
        switch (predefinedPackageCatalog)
        {
        case winrt::Microsoft::Management::Deployment::PredefinedPackageCatalog::OpenWindowsCatalog:
            sourceDetails = GetWellKnownSourceDetails(::AppInstaller::Repository::WellKnownSource::WinGet);
            break;
        case winrt::Microsoft::Management::Deployment::PredefinedPackageCatalog::MicrosoftStore:
            sourceDetails = GetWellKnownSourceDetails(::AppInstaller::Repository::WellKnownSource::MicrosoftStore);
            break;
        default:
            throw hresult_invalid_argument();
        }
        auto packageCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>>();
        packageCatalogInfo->Initialize(sourceDetails);
        auto packageCatalogRef = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference>>();
        packageCatalogRef->Initialize(*packageCatalogInfo);
        return *packageCatalogRef;
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageManager::GetLocalPackageCatalog(winrt::Microsoft::Management::Deployment::LocalPackageCatalog const& localPackageCatalog)
    {
        ::AppInstaller::Repository::SourceDetails sourceDetails;
        switch (localPackageCatalog)
        {
        case winrt::Microsoft::Management::Deployment::LocalPackageCatalog::InstalledPackages:
            sourceDetails = GetPredefinedSourceDetails(::AppInstaller::Repository::PredefinedSource::Installed);
            break;
        case winrt::Microsoft::Management::Deployment::LocalPackageCatalog::InstallingPackages:
            sourceDetails = GetPredefinedSourceDetails(::AppInstaller::Repository::PredefinedSource::Installing);
            break;
        default:
            throw hresult_invalid_argument();
        }
        auto packageCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>>();
        packageCatalogInfo->Initialize(sourceDetails);
        auto packageCatalogRef = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference>>();
        packageCatalogRef->Initialize(*packageCatalogInfo);
        return *packageCatalogRef;
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageManager::GetPackageCatalogByName(hstring const& catalogName)
    {
        std::optional<::AppInstaller::Repository::SourceDetails> source = ::AppInstaller::Repository::GetSource(winrt::to_string(catalogName));
        // Create the catalog object if the source is found, otherwise return null. Don't throw.
        if (source.has_value())
        {
            auto packageCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>>();
            packageCatalogInfo->Initialize(source.value());
            auto packageCatalogRef = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference>>();
            packageCatalogRef->Initialize(*packageCatalogInfo);
            return *packageCatalogRef;
        }
        else
        {
            return nullptr;
        }
    }
    void AddPackageManifestToContext(winrt::Microsoft::Management::Deployment::PackageVersionInfo packageVersionInfo, ::AppInstaller::CLI::Execution::Context* context)
    {
        winrt::Microsoft::Management::Deployment::implementation::PackageVersionInfo* packageVersionInfoImpl = get_self<winrt::Microsoft::Management::Deployment::implementation::PackageVersionInfo>(packageVersionInfo);
        std::shared_ptr<::AppInstaller::Repository::IPackageVersion> internalPackageVersion = packageVersionInfoImpl->GetRepositoryPackageVersion();
        ::AppInstaller::Manifest::Manifest manifest = internalPackageVersion->GetManifest();

        std::string targetLocale;
        if (context->Args.Contains(::AppInstaller::CLI::Execution::Args::Type::Locale))
        {
            targetLocale = context->Args.GetArg(::AppInstaller::CLI::Execution::Args::Type::Locale);
        }
        manifest.ApplyLocale(targetLocale);

        ::AppInstaller::Logging::Telemetry().LogManifestFields(manifest.Id, manifest.DefaultLocalization.Get<::AppInstaller::Manifest::Localization::PackageName>(), manifest.Version);

        context->Add<::AppInstaller::CLI::Execution::Data::Manifest>(std::move(manifest));
        context->Add<::AppInstaller::CLI::Execution::Data::PackageVersion>(std::move(internalPackageVersion));
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageManager::CreateCompositePackageCatalog(winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions const& options)
    {
        for (uint32_t i = 0; i < options.Catalogs().Size(); ++i)
        {
            auto catalog = options.Catalogs().GetAt(i);
            if (catalog.IsComposite())
            {
                // Can't make a composite source out of a source that's already a composite.
                throw hresult_invalid_argument();
            }
        }
        auto packageCatalogImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference>>();
        packageCatalogImpl->Initialize(options);
        return *packageCatalogImpl;
    }

    winrt::Microsoft::Management::Deployment::InstallResult GetInstallResult(::Workflow::ExecutionStage executionStage, winrt::hresult terminationHR, winrt::hstring correlationData, bool rebootRequired)
    {
        winrt::Microsoft::Management::Deployment::InstallResultStatus installResultStatus = GetInstallResultStatus(executionStage, terminationHR);
        auto installResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::InstallResult>>();
        installResult->Initialize(installResultStatus, terminationHR, correlationData, rebootRequired);
        return *installResult;
    }

    std::optional<winrt::Microsoft::Management::Deployment::InstallProgress> GetProgress(
        ::AppInstaller::ReportType reportType,
        uint64_t current,
        uint64_t maximum,
        ::AppInstaller::ProgressType progressType,
        ::Workflow::ExecutionStage executionPhase)
    {
        bool reportProgress = false;
        PackageInstallProgressState progressState = PackageInstallProgressState::Queued;
        double downloadProgress = 0;
        double installProgress = 0;
        uint64_t downloadBytesDownloaded = 0;
        uint64_t downloadBytesRequired = 0;
        switch (executionPhase)
        {
        case ::Workflow::ExecutionStage::Initial:
        case ::Workflow::ExecutionStage::ParseArgs:
        case ::Workflow::ExecutionStage::Discovery:
            // We already reported queued progress up front.
            break;
        case ::Workflow::ExecutionStage::Download:
            progressState = PackageInstallProgressState::Downloading;
            if (reportType == ::AppInstaller::ReportType::BeginProgress)
            {
                reportProgress = true;
            }
            else if (progressType == ::AppInstaller::ProgressType::Bytes)
            {
                downloadBytesDownloaded = current;
                downloadBytesRequired = maximum;
                if (maximum > 0 && maximum >= current)
                {
                    reportProgress = true;
                    downloadProgress = static_cast<double>(current) / static_cast<double>(maximum);
                }
            }
            break;
        case ::Workflow::ExecutionStage::PreExecution:
            // Wait until installer starts to report Installing.
            break;
        case ::Workflow::ExecutionStage::Execution:
            progressState = PackageInstallProgressState::Installing;
            downloadProgress = 1;
            if (reportType == ::AppInstaller::ReportType::ExecutionPhaseUpdate)
            {
                // Install is starting. Send progress so callers know the AsyncOperation can't be cancelled.
                reportProgress = true;
            }
            else if (reportType == ::AppInstaller::ReportType::EndProgress)
            {
                // Install is "finished". May not have succeeded.
                reportProgress = true;
                installProgress = 1;
            }
            else if (progressType == ::AppInstaller::ProgressType::Percent)
            {
                if (maximum > 0 && maximum >= current)
                {
                    // Install is progressing
                    reportProgress = true;
                    installProgress = static_cast<double>(current) / static_cast<double>(maximum);
                }
            }
            break;
        case ::Workflow::ExecutionStage::PostExecution:
            if (reportType == ::AppInstaller::ReportType::ExecutionPhaseUpdate)
            {
                // Send PostInstall progress when it switches to PostExecution phase.
                reportProgress = true;
                progressState = PackageInstallProgressState::PostInstall;
                downloadProgress = 1;
                installProgress = 1;
            }
            break;
        }
        if (reportProgress)
        {
            winrt::Microsoft::Management::Deployment::InstallProgress contextProgress{ progressState, downloadBytesDownloaded, downloadBytesRequired, downloadProgress, installProgress };
            return contextProgress;
        }
        else
        {
            return {};
        }
    }
    
    Microsoft::Management::Deployment::PackageVersionInfo GetPackageVersionInfo(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::InstallOptions options)
    {
        Microsoft::Management::Deployment::PackageVersionInfo packageVersionInfo{ nullptr };

        winrt::Microsoft::Management::Deployment::PackageVersionId versionId = (options) ? options.PackageVersionId() : nullptr;
        // If the version of the package is specified use that, otherwise use the default.
        if (versionId)
        {
            packageVersionInfo = package.GetPackageVersionInfo(versionId);
        }
        else
        {
            packageVersionInfo = package.DefaultInstallVersion();
        }
        // If the specified version wasn't found then return a failure. This is unusual, since all packages that came from a non-local catalog have a default version,
        // and the versionId is strongly typed and comes from the CatalogPackage.GetAvailableVersions.
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_NO_MANIFEST_FOUND, !packageVersionInfo);
        return packageVersionInfo;
    }

    std::unique_ptr<::AppInstaller::COMContext> CreateContextFromInstallOptions(
        winrt::Microsoft::Management::Deployment::CatalogPackage package, 
        winrt::Microsoft::Management::Deployment::InstallOptions options, 
        std::wstring callerProcessInfoString)
    {
        std::unique_ptr<::AppInstaller::COMContext> context = std::make_unique<::AppInstaller::COMContext>();
        hstring correlationData = (options) ? options.CorrelationData() : L"";
        context->SetLoggerContext(correlationData, ::AppInstaller::Utility::ConvertToUTF8(callerProcessInfoString));

        // Convert the options to arguments for the installer.
        if (options)
        {
            if (!options.LogOutputPath().empty())
            {
                context->Args.AddArg(Execution::Args::Type::Log, ::AppInstaller::Utility::ConvertToUTF8(options.LogOutputPath()));
                context->Args.AddArg(Execution::Args::Type::VerboseLogs);
            }
            if (options.AllowHashMismatch())
            {
                context->Args.AddArg(Execution::Args::Type::HashOverride);
            }

            // If the PackageInstallScope is anything other than ::Any then set it as a requirement.
            if (options.PackageInstallScope() == PackageInstallScope::System)
            {
                context->Args.AddArg(Execution::Args::Type::InstallScope, ScopeToString(::AppInstaller::Manifest::ScopeEnum::Machine));
            }
            else if (options.PackageInstallScope() == PackageInstallScope::User)
            {
                context->Args.AddArg(Execution::Args::Type::InstallScope, ScopeToString(::AppInstaller::Manifest::ScopeEnum::User));
            }

            if (options.PackageInstallMode() == PackageInstallMode::Interactive)
            {
                context->Args.AddArg(Execution::Args::Type::Interactive);
            }
            else if (options.PackageInstallMode() == PackageInstallMode::Silent)
            {
                context->Args.AddArg(Execution::Args::Type::Silent);
            }

            if (!options.PreferredInstallLocation().empty())
            {
                context->Args.AddArg(Execution::Args::Type::InstallLocation, ::AppInstaller::Utility::ConvertToUTF8(options.PreferredInstallLocation()));
            }

            if (!options.ReplacementInstallerArguments().empty())
            {
                context->Args.AddArg(Execution::Args::Type::Override, ::AppInstaller::Utility::ConvertToUTF8(options.ReplacementInstallerArguments()));
            }
        }

        // If the version of the package is specified use that, otherwise use the default.
        Microsoft::Management::Deployment::PackageVersionInfo packageVersionInfo = GetPackageVersionInfo(package, options);
        AddPackageManifestToContext(packageVersionInfo, context.get());

        // Note: AdditionalPackageCatalogArguments is not needed during install since the manifest is already known so no additional calls to the source are needed. The property is deprecated.
        return context;
    }

    std::shared_ptr<Execution::OrchestratorQueueItem> GetExistingQueueItemForPackage(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::PackageCatalogInfo catalogInfo)
    {
        std::shared_ptr<Execution::OrchestratorQueueItem> queueItem = nullptr;
        std::unique_ptr<::AppInstaller::COMContext> context = std::make_unique<::AppInstaller::COMContext>();
        if (catalogInfo)
        {
            // If the caller has passed in the catalog they expect the package to have come from, then only look for an install from that catalog.
            // Fail if they've used a catalog that doesn't have an Id. This can currently happen for Info objects that come from PackageCatalogReference objects for REST catalogs.
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS, catalogInfo.Id().empty());
            auto searchItem = Execution::OrchestratorQueueItemFactory::CreateItemForInstall(std::wstring{ package.Id() }, std::wstring{ catalogInfo.Id() }, std::move(context));
            queueItem = Execution::ContextOrchestrator::Instance().GetQueueItem(searchItem->GetId());
            return queueItem;
        }

        // If the caller has not specified the catalog, then check InstalledVersion. When the package comes from the Installing catalog the PackageCatalog
        // of the InstalledVersion will be set to the original catalog that the install was from, so checking the InstalledVersion first is most likely to 
        // find a result.
        Microsoft::Management::Deployment::PackageVersionInfo installedVersionInfo = package.InstalledVersion();
        if (installedVersionInfo)
        {
            auto searchItem = Execution::OrchestratorQueueItemFactory::CreateItemForInstall(std::wstring{ package.Id() }, std::wstring{ installedVersionInfo.PackageCatalog().Info().Id() }, std::move(context));
            queueItem = Execution::ContextOrchestrator::Instance().GetQueueItem(searchItem->GetId());
            if (queueItem)
            {
                return queueItem;
            }
        }

        // If InstalledVersion was not found, check DefaultInstallVersion
        Microsoft::Management::Deployment::PackageVersionInfo defaultInstallVersionInfo = package.DefaultInstallVersion();
        if (defaultInstallVersionInfo)
        {
            auto searchItem = Execution::OrchestratorQueueItemFactory::CreateItemForInstall(std::wstring{ package.Id() }, std::wstring{ defaultInstallVersionInfo.PackageCatalog().Info().Id() }, std::move(context));
            queueItem = Execution::ContextOrchestrator::Instance().GetQueueItem(searchItem->GetId());
            if (queueItem)
            {
                return queueItem;
            }
        }

        // Finally check all catalogs in AvailableVersions.
        for (Microsoft::Management::Deployment::PackageVersionId versionId : package.AvailableVersions())
        {
            auto searchItem = Execution::OrchestratorQueueItemFactory::CreateItemForInstall(std::wstring{ package.Id() }, std::wstring{ package.GetPackageVersionInfo(versionId).PackageCatalog().Info().Id() }, std::move(context));
            queueItem = Execution::ContextOrchestrator::Instance().GetQueueItem(searchItem->GetId());
            if (queueItem)
            {
                return queueItem;
            }
        }
        return nullptr;
    }

    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> GetInstallOperation(
        bool addToQueue, 
        winrt::Microsoft::Management::Deployment::CatalogPackage package, 
        winrt::Microsoft::Management::Deployment::InstallOptions options, 
        winrt::Microsoft::Management::Deployment::PackageCatalogInfo catalogInfo)
    {
        winrt::hresult terminationHR = S_OK;
        hstring correlationData = (options) ? options.CorrelationData() : L"";
        ::Workflow::ExecutionStage executionStage = ::Workflow::ExecutionStage::Initial;

        #define WINGET_RETURN_INSTALL_RESULT_IF(installResult, boolVal) { if(boolVal) { co_return installResult; }}
        #define WINGET_RETURN_INSTALL_RESULT_HR(hr) { WINGET_RETURN_INSTALL_RESULT_IF(GetInstallResult(executionStage, hr, correlationData, false), true) }
        #define WINGET_RETURN_INSTALL_RESULT_HR_IF(hr, boolVal) { if(boolVal) { WINGET_RETURN_INSTALL_RESULT_HR(hr) }}
        #define WINGET_RETURN_INSTALL_RESULT_HR_IF_FAILED(hr) { WINGET_RETURN_INSTALL_RESULT_HR_IF(hr, FAILED(hr)) }

        // options and catalog can both be null, package must be set.
        WINGET_RETURN_INSTALL_RESULT_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS, !package);

        try
        {
            auto report_progress{ co_await winrt::get_progress_token() };
            auto cancellationToken{ co_await winrt::get_cancellation_token() };

            wil::unique_event progressEvent{ wil::EventOptions::None };

            std::shared_ptr<Execution::OrchestratorQueueItem> queueItem = nullptr;
            if (addToQueue)
            {
                // Check for permissions and get caller info for telemetry.
                // This must be done before any co_awaits since it requires info from the rpc caller thread.
                std::optional<DWORD> callerProcessId = GetCallerProcessId();
                WINGET_RETURN_INSTALL_RESULT_HR_IF(E_ACCESSDENIED, !callerProcessId.has_value());
                WINGET_RETURN_INSTALL_RESULT_HR_IF_FAILED(EnsureProcessHasCapability(Capability::PackageManagement, callerProcessId.value()));
                std::wstring callerProcessInfoString = TryGetCallerProcessInfo(callerProcessId.value());

                // co_await does not guarantee that it's on a background thread, so do so explicitly.
                co_await winrt::resume_background();

                Microsoft::Management::Deployment::PackageVersionInfo packageVersionInfo = GetPackageVersionInfo(package, options);
                std::unique_ptr<::AppInstaller::COMContext> comContext = CreateContextFromInstallOptions(package, options, callerProcessInfoString);
                queueItem = Execution::OrchestratorQueueItemFactory::CreateItemForInstall(std::wstring{ package.Id() }, std::wstring{ packageVersionInfo.PackageCatalog().Info().Id() }, std::move(comContext));
                Execution::ContextOrchestrator::Instance().EnqueueAndRunItem(queueItem);

                InstallProgress queuedProgress{ PackageInstallProgressState::Queued, 0, 0, 0 };
                report_progress(queuedProgress);
            }
            else
            {
                WINGET_RETURN_INSTALL_RESULT_HR_IF_FAILED(EnsureComCallerHasCapability(Capability::PackageQuery));

                queueItem = GetExistingQueueItemForPackage(package, catalogInfo);
                WINGET_RETURN_INSTALL_RESULT_IF(nullptr, queueItem == nullptr);

                // correlation data is not passed in when retrieving an existing queue item, so get it from the existing context.
                correlationData = hstring(queueItem->GetContext().GetCorrelationJson());

                // co_await does not guarantee that it's on a background thread, so do so explicitly.
                co_await winrt::resume_background();
            }

            std::atomic<winrt::Microsoft::Management::Deployment::InstallProgress> installProgress;
            queueItem->GetContext().AddProgressCallbackFunction([&installProgress, &progressEvent](
                ::AppInstaller::ReportType reportType,
                uint64_t current,
                uint64_t maximum,
                ::AppInstaller::ProgressType progressType,
                ::Workflow::ExecutionStage executionPhase)
                {
                    std::optional<winrt::Microsoft::Management::Deployment::InstallProgress> installProgressOptional = GetProgress(reportType, current, maximum, progressType, executionPhase);
                    if (installProgressOptional.has_value())
                    {
                        installProgress = installProgressOptional.value();
                        ::SetEvent(progressEvent.get());
                    }
                    return;
                }
            );
            cancellationToken.callback([&queueItem]
                {
                    // The cancellation of the AsyncOperation on the client triggers Cancel which causes the Execute to end.
                    Execution::ContextOrchestrator::Instance().CancelQueueItem(*queueItem);
                });

            // Wait for completion or progress events.
            // Waiting for both on the same thread ensures that progress is never reported after the async operation itself has completed.
            bool completionEventFired = false;
            HANDLE operationEvents[2];
            operationEvents[0] = progressEvent.get();
            operationEvents[1] = queueItem->GetCompletedEvent().get();
            while (!completionEventFired)
            {
                DWORD dwEvent = WaitForMultipleObjects(
                    2 /* number of events */,
                    operationEvents /* event array */,
                    FALSE /* bWaitAll, FALSE to wake on any event */,
                    INFINITE /* wait until operation completion */);

                switch (dwEvent)
                {
                    // operationEvents[0] was signaled, progress
                case WAIT_OBJECT_0 + 0:
                    // The report_progress call will hang when making callbacks to suspended processes so it's important that this is now on a background thread.
                    // Progress events are not queued - some will be missed if multiple progress events are fired from the ComContext to the callback 
                    // while the report_progress call is hung\in progress.
                    // Duplicate progress events can be fired if another progress event comes from the ComContext to the callback after the listener
                    // has been awaked, but before it has gotten the installProgress.
                    report_progress(installProgress);
                    break;

                    // operationEvents[1] was signaled, operation completed
                case WAIT_OBJECT_0 + 1:
                    completionEventFired = true;
                    break;

                    // Return value is invalid.
                default:
                    THROW_LAST_ERROR();
                }
            }

            // The install command has finished, check for success/failure and how far it got.
            terminationHR = queueItem->GetContext().GetTerminationHR();
            executionStage = queueItem->GetContext().GetExecutionStage();
        }
        WINGET_CATCH_STORE(terminationHR, APPINSTALLER_CLI_ERROR_COMMAND_FAILED);

        // TODO - RebootRequired not yet populated, msi arguments not returned from Execute.
        WINGET_RETURN_INSTALL_RESULT_HR(terminationHR);
    }

    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> PackageManager::InstallPackageAsync(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::InstallOptions options)
    {
        return GetInstallOperation(true, package, options, nullptr);
    }

    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> PackageManager::GetInstallProgress(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::PackageCatalogInfo catalogInfo)
    {
        return GetInstallOperation(false, package, nullptr, catalogInfo);
    }

    CoCreatableCppWinRtClassWithCLSID(PackageManager, 1, &PackageManagerCLSID1);
    CoCreatableCppWinRtClassWithCLSID(PackageManager, 2, &PackageManagerCLSID2);
}
