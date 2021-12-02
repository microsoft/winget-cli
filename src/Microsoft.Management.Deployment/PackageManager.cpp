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
using namespace ::AppInstaller::CLI::Execution;

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::PackageCatalogReference> PackageManager::GetPackageCatalogs()
    {
        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::PackageCatalogReference> catalogs{ winrt::single_threaded_vector<Microsoft::Management::Deployment::PackageCatalogReference>() };
        std::vector<::AppInstaller::Repository::SourceDetails> sources = ::AppInstaller::Repository::Source::GetCurrentSources();
        for (uint32_t i = 0; i < sources.size(); i++)
        {
            auto packageCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>>();
            ::AppInstaller::Repository::Source sourceReference{ sources.at(i).Name };
            packageCatalogInfo->Initialize(sourceReference.GetDetails());
            auto packageCatalogRef = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference>>();
            packageCatalogRef->Initialize(*packageCatalogInfo, sourceReference);
            catalogs.Append(*packageCatalogRef);
        }
        return catalogs.GetView();
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageManager::GetPredefinedPackageCatalog(winrt::Microsoft::Management::Deployment::PredefinedPackageCatalog const& predefinedPackageCatalog)
    {
        ::AppInstaller::Repository::Source source;
        switch (predefinedPackageCatalog)
        {
        case winrt::Microsoft::Management::Deployment::PredefinedPackageCatalog::OpenWindowsCatalog:
            source = ::AppInstaller::Repository::Source{ ::AppInstaller::Repository::WellKnownSource::WinGet };
            break;
        case winrt::Microsoft::Management::Deployment::PredefinedPackageCatalog::MicrosoftStore:
            source = ::AppInstaller::Repository::Source{ ::AppInstaller::Repository::WellKnownSource::MicrosoftStore };
            break;
        case winrt::Microsoft::Management::Deployment::PredefinedPackageCatalog::DesktopFrameworks:
            source = ::AppInstaller::Repository::Source{ ::AppInstaller::Repository::WellKnownSource::DesktopFrameworks };
            break;
        default:
            throw hresult_invalid_argument();
        }
        auto packageCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>>();
        packageCatalogInfo->Initialize(source.GetDetails());
        auto packageCatalogRef = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference>>();
        packageCatalogRef->Initialize(*packageCatalogInfo, source);
        return *packageCatalogRef;
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageManager::GetLocalPackageCatalog(winrt::Microsoft::Management::Deployment::LocalPackageCatalog const& localPackageCatalog)
    {
        ::AppInstaller::Repository::Source source;
        switch (localPackageCatalog)
        {
        case winrt::Microsoft::Management::Deployment::LocalPackageCatalog::InstalledPackages:
            source = ::AppInstaller::Repository::Source{ ::AppInstaller::Repository::PredefinedSource::Installed };
            break;
        case winrt::Microsoft::Management::Deployment::LocalPackageCatalog::InstallingPackages:
            source = ::AppInstaller::Repository::Source{ ::AppInstaller::Repository::PredefinedSource::Installing };
            break;
        default:
            throw hresult_invalid_argument();
        }
        auto packageCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>>();
        packageCatalogInfo->Initialize(source.GetDetails());
        auto packageCatalogRef = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference>>();
        packageCatalogRef->Initialize(*packageCatalogInfo, source);
        return *packageCatalogRef;
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageManager::GetPackageCatalogByName(hstring const& catalogName)
    {
        std::string name = winrt::to_string(catalogName);
        if (name.empty())
        {
            return nullptr;
        }

        ::AppInstaller::Repository::Source source{ name };
        // Create the catalog object if the source is found, otherwise return null. Don't throw.
        if (source)
        {
            auto packageCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>>();
            packageCatalogInfo->Initialize(source.GetDetails());
            auto packageCatalogRef = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference>>();
            packageCatalogRef->Initialize(*packageCatalogInfo, source);
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

        context->GetThreadGlobals().GetTelemetryLogger().LogManifestFields(manifest.Id, manifest.DefaultLocalization.Get<::AppInstaller::Manifest::Localization::PackageName>(), manifest.Version);

        context->Add<::AppInstaller::CLI::Execution::Data::Manifest>(std::move(manifest));
        context->Add<::AppInstaller::CLI::Execution::Data::PackageVersion>(std::move(internalPackageVersion));
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageManager::CreateCompositePackageCatalog(winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions const& options)
    {
        if (!options)
        {
            // Can't make a composite source if the options aren't specified.
            throw hresult_invalid_argument();
        }

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
        ReportType reportType,
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
            if (reportType == ReportType::BeginProgress)
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
            if (reportType == ReportType::ExecutionPhaseUpdate)
            {
                // Install is starting. Send progress so callers know the AsyncOperation can't be cancelled.
                reportProgress = true;
            }
            else if (reportType == ReportType::EndProgress)
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
            if (reportType == ReportType::ExecutionPhaseUpdate)
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

    std::unique_ptr<COMContext> CreateContextFromInstallOptions(
        winrt::Microsoft::Management::Deployment::CatalogPackage package,
        winrt::Microsoft::Management::Deployment::InstallOptions options,
        std::wstring callerProcessInfoString)
    {
        std::unique_ptr<COMContext> context = std::make_unique<COMContext>();
        hstring correlationData = (options) ? options.CorrelationData() : L"";
        context->SetContextLoggers(correlationData, ::AppInstaller::Utility::ConvertToUTF8(callerProcessInfoString));

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

            if (options.AllowedArchitectures().Size() != 0)
            {
                std::vector<AppInstaller::Utility::Architecture> allowedArchitectures;
                for (auto architecture : options.AllowedArchitectures())
                {
                    auto convertedArchitecture = GetUtilityArchitecture(architecture);
                    if (convertedArchitecture)
                    {
                        allowedArchitectures.push_back(convertedArchitecture.value());
                    }
                }
                context->Add<Data::AllowedArchitectures>(std::move(allowedArchitectures));
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
        std::unique_ptr<COMContext> context = std::make_unique<COMContext>();
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
        bool canCancelQueueItem,
        std::shared_ptr<Execution::OrchestratorQueueItem> queueItemParam,
        winrt::Microsoft::Management::Deployment::CatalogPackage package = nullptr,
        winrt::Microsoft::Management::Deployment::InstallOptions options = nullptr,
        std::wstring callerProcessInfoString = {})
    {
        winrt::hresult terminationHR = S_OK;
        hstring correlationData = (options) ? options.CorrelationData() : L"";
        ::Workflow::ExecutionStage executionStage = ::Workflow::ExecutionStage::Initial;

        try
        {
            // re-scope the parameter to inside the try block to avoid lifetime management issues.
            std::shared_ptr<Execution::OrchestratorQueueItem> queueItem = std::move(queueItemParam);

            auto report_progress{ co_await winrt::get_progress_token() };
            auto cancellationToken{ co_await winrt::get_cancellation_token() };
            // co_await does not guarantee that it's on a background thread, so do so explicitly.
            co_await winrt::resume_background();

            if (queueItem == nullptr)
            {
                Microsoft::Management::Deployment::PackageVersionInfo packageVersionInfo = GetPackageVersionInfo(package, options);
                std::unique_ptr<COMContext> comContext = CreateContextFromInstallOptions(package, options, callerProcessInfoString);
                queueItem = Execution::OrchestratorQueueItemFactory::CreateItemForInstall(std::wstring{ package.Id() }, std::wstring{ packageVersionInfo.PackageCatalog().Info().Id() }, std::move(comContext));
                Execution::ContextOrchestrator::Instance().EnqueueAndRunItem(queueItem);

                InstallProgress queuedProgress{ PackageInstallProgressState::Queued, 0, 0, 0 };
                report_progress(queuedProgress);
            }
            {
                // correlation data is not passed in when retrieving an existing queue item, so get it from the existing context.
                correlationData = hstring(queueItem->GetContext().GetCorrelationJson());
            }

            wil::unique_event progressEvent{ wil::EventOptions::None };

            std::atomic<winrt::Microsoft::Management::Deployment::InstallProgress> installProgress;
            queueItem->GetContext().AddProgressCallbackFunction([&installProgress, &progressEvent](
                ReportType reportType,
                uint64_t current,
                uint64_t maximum,
                ::AppInstaller::ProgressType progressType,
                ::Workflow::ExecutionStage executionPhase)
                {
                    std::optional<winrt::Microsoft::Management::Deployment::InstallProgress> installProgressOptional = GetProgress(reportType, current, maximum, progressType, executionPhase);
                    if (installProgressOptional.has_value())
                    {
                        installProgress = installProgressOptional.value();
                        progressEvent.SetEvent();
                    }
                    return;
                }
            );

            std::weak_ptr<Execution::OrchestratorQueueItem> weakQueueItem(queueItem);
            cancellationToken.callback([weakQueueItem, &canCancelQueueItem]
                {
                    if (canCancelQueueItem)
                    {
                        auto strongQueueItem = weakQueueItem.lock();
                        if (strongQueueItem) {
                            // The cancellation of the AsyncOperation on the client triggers Cancel which causes the Execute to end.
                            Execution::ContextOrchestrator::Instance().CancelQueueItem(*strongQueueItem);
                        }
                    }
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
                    _countof(operationEvents) /* number of events */,
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

            if (completionEventFired)
            {
                // The install command has finished, check for success/failure and how far it got.
                terminationHR = queueItem->GetContext().GetTerminationHR();
                executionStage = queueItem->GetContext().GetExecutionStage();
            }
        }
        WINGET_CATCH_STORE(terminationHR, APPINSTALLER_CLI_ERROR_COMMAND_FAILED);

        // TODO - RebootRequired not yet populated, msi arguments not returned from Execute.
        co_return GetInstallResult(executionStage, terminationHR, correlationData, false);
    }

    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> GetEmptyAsynchronousResultForInstallOperation(
        HRESULT hr,
        hstring correlationData)
    {
        // If a function uses co_await or co_return (i.e. if it is a co_routine), it cannot use return directly.
        // This helper helps a function that is not a coroutine itself to return errors asynchronously.
        co_return GetInstallResult(::Workflow::ExecutionStage::Initial, hr, correlationData, false);
    }

#define WINGET_RETURN_INSTALL_RESULT_HR_IF(hr, boolVal) { if(boolVal) { return GetEmptyAsynchronousResultForInstallOperation(hr, correlationData); }}
#define WINGET_RETURN_INSTALL_RESULT_HR_IF_FAILED(hr) { WINGET_RETURN_INSTALL_RESULT_HR_IF(hr, FAILED(hr)) }

    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> PackageManager::InstallPackageAsync(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::InstallOptions options)
    {
        hstring correlationData = (options) ? options.CorrelationData() : L"";

        // options and catalog can both be null, package must be set.
        WINGET_RETURN_INSTALL_RESULT_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS, !package);

        HRESULT hr = S_OK;
        std::wstring callerProcessInfoString;
        try
        {
            // Check for permissions and get caller info for telemetry.
            // This must be done before any co_awaits since it requires info from the rpc caller thread.
            auto [hrGetCallerId, callerProcessId] = GetCallerProcessId();
            WINGET_RETURN_INSTALL_RESULT_HR_IF_FAILED(hrGetCallerId);
            WINGET_RETURN_INSTALL_RESULT_HR_IF_FAILED(EnsureProcessHasCapability(Capability::PackageManagement, callerProcessId));
            callerProcessInfoString = TryGetCallerProcessInfo(callerProcessId);
        }
        WINGET_CATCH_STORE(hr, APPINSTALLER_CLI_ERROR_COMMAND_FAILED);
        WINGET_RETURN_INSTALL_RESULT_HR_IF_FAILED(hr);

        return GetInstallOperation(true /*canCancelQueueItem*/, nullptr /*queueItem*/, package, options, std::move(callerProcessInfoString));
    }

    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> PackageManager::GetInstallProgress(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::PackageCatalogInfo catalogInfo)
    {
        hstring correlationData;
        WINGET_RETURN_INSTALL_RESULT_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS, !package);

        HRESULT hr = S_OK;
        std::shared_ptr<Execution::OrchestratorQueueItem> queueItem = nullptr;
        bool canCancelQueueItem = false;
        try
        {
            // Check for permissions
            // This must be done before any co_awaits since it requires info from the rpc caller thread.
            auto [hrGetCallerId, callerProcessId] = GetCallerProcessId();
            WINGET_RETURN_INSTALL_RESULT_HR_IF_FAILED(hrGetCallerId);
            canCancelQueueItem = SUCCEEDED(EnsureProcessHasCapability(Capability::PackageManagement, callerProcessId));
            if (!canCancelQueueItem)
            {
                WINGET_RETURN_INSTALL_RESULT_HR_IF_FAILED(EnsureProcessHasCapability(Capability::PackageQuery, callerProcessId));
            }

            // Get the queueItem synchronously.
            queueItem = GetExistingQueueItemForPackage(package, catalogInfo);
            if (queueItem == nullptr)
            {
                return nullptr;
            }
        }
        WINGET_CATCH_STORE(hr, APPINSTALLER_CLI_ERROR_COMMAND_FAILED);
        WINGET_RETURN_INSTALL_RESULT_HR_IF_FAILED(hr);

        return GetInstallOperation(canCancelQueueItem, std::move(queueItem));
    }

    CoCreatableMicrosoftManagementDeploymentClass(PackageManager);
}
