#include "pch.h"
#include "Public/AppInstallerCLICore.h"
#include "Microsoft/PredefinedInstalledSourceFactory.h"
#include "Commands/RootCommand.h"
#include "ComContext.h"
#include "ExecutionContext.h"
#include "Workflows/WorkflowBase.h"
#include <winget/UserSettings.h>
#include "Commands/InstallCommand.h"
#include <AppInstallerTelemetry.h>
#include <AppInstallerErrors.h>
#include "AppInstaller.h"
#include "AppInstaller.g.cpp"
#include "InstallResult.h"
#include "AppCatalog.h"
#include "PackageVersionInfo.h"
#include "PackageVersionId.h"

using namespace std::literals::chrono_literals;

namespace winrt::Microsoft::Management::Deployment::implementation
{
    Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::AppCatalog> AppInstaller::GetAppCatalogs()
    {
        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::AppCatalog> catalogs{ winrt::single_threaded_vector<Microsoft::Management::Deployment::AppCatalog>() };
        std::vector<::AppInstaller::Repository::SourceDetails> sources = ::AppInstaller::Repository::GetSources();
        for (uint32_t i = 0; i < sources.size(); i++)
        {
            winrt::Microsoft::Management::Deployment::AppCatalog appCatalog{ nullptr };
            appCatalog = winrt::make<winrt::Microsoft::Management::Deployment::implementation::AppCatalog>(winrt::to_hstring(sources.at(i).Name));
            catalogs.Append(appCatalog);
        }
        return catalogs.GetView();
    }
    Microsoft::Management::Deployment::AppCatalog AppInstaller::GetAppCatalog(Microsoft::Management::Deployment::PredefinedAppCatalog const& predefinedAppCatalog)
    {
        winrt::Microsoft::Management::Deployment::AppCatalog appCatalog{ nullptr };
        appCatalog = winrt::make<winrt::Microsoft::Management::Deployment::implementation::AppCatalog>(predefinedAppCatalog);
        return appCatalog;
    }
    Microsoft::Management::Deployment::AppCatalog AppInstaller::GetAppCatalogById(hstring const& catalogId)
    {
        winrt::Microsoft::Management::Deployment::AppCatalog appCatalog{ nullptr };
        appCatalog = winrt::make<winrt::Microsoft::Management::Deployment::implementation::AppCatalog>(catalogId);
        return appCatalog;
    }
    Microsoft::Management::Deployment::AppCatalog AppInstaller::GetCompositeAppCatalog(Microsoft::Management::Deployment::GetCompositeAppCatalogOptions const& options)
    {
        if (options.Catalogs().Size() == 0)
        {
            throw hresult_invalid_argument();
        }
        if (options.Catalogs().Size() > 2)
        {
            throw hresult_not_implemented();
        }
        bool includeInstalledCatalog = false;
        bool includeNonLocalCatalog = false;
        for (int i = 0; i < options.Catalogs().Size(); ++i)
        {
            auto catalog = options.Catalogs().GetAt(i);
            if (catalog.IsComposite())
            {
                throw hresult_invalid_argument();
            }
            if (winrt::to_string(catalog.Info().Name()).compare(::AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Type()) == 0)
            {
                includeInstalledCatalog = true;
            }
            else
            {
                includeNonLocalCatalog = true;
            }
        }
        if (!includeInstalledCatalog || !includeNonLocalCatalog)
        {
            throw hresult_not_implemented();
        }
        winrt::Microsoft::Management::Deployment::AppCatalog appCatalog{ nullptr };
        appCatalog = winrt::make<winrt::Microsoft::Management::Deployment::implementation::AppCatalog>(options);
        return appCatalog;
    }

    winrt::handle AppInstaller::g_event;
    Microsoft::Management::Deployment::InstallProgress AppInstaller::g_progressValue;

    void ProgressCallback(::AppInstaller::ReportType reportType, uint64_t current, uint64_t maximum, ::AppInstaller::ProgressType progressType, ::AppInstaller::CLI::Workflow::ExecutionStage executionPhase)
    {
        AppInstallProgressState progressState = AppInstallProgressState::Queued;
        float downloadPercentage = 0;
        float downloadBytesDownloaded = 0;
        float downloadBytesRequired = 0;
        switch (executionPhase)
        {
        case ::AppInstaller::CLI::Workflow::ExecutionStage::Initial:
        case ::AppInstaller::CLI::Workflow::ExecutionStage::ParseArgs:
        case ::AppInstaller::CLI::Workflow::ExecutionStage::Discovery:
            progressState = AppInstallProgressState::Queued;
            downloadPercentage = 0;
            break;
        case ::AppInstaller::CLI::Workflow::ExecutionStage::Download:
            progressState = AppInstallProgressState::Downloading;
            if (progressType == ::AppInstaller::ProgressType::Bytes)
            {
                downloadBytesDownloaded = current;
                downloadBytesRequired = maximum;
                if (downloadBytesRequired > 0)
                {
                    downloadPercentage = current / maximum;
                }
            }
            break;
        case ::AppInstaller::CLI::Workflow::ExecutionStage::PreExecution:
        case ::AppInstaller::CLI::Workflow::ExecutionStage::Execution:
            progressState = AppInstallProgressState::Installing;
            downloadPercentage = 100;
            break;
        case ::AppInstaller::CLI::Workflow::ExecutionStage::PostExecution:
            progressState = AppInstallProgressState::PostInstall;
            downloadPercentage = 100;
            break;
        }
        InstallProgress queuedProgress{ progressState, downloadBytesDownloaded, downloadBytesRequired, downloadPercentage };
        Microsoft::Management::Deployment::implementation::AppInstaller::SetProgressValue(queuedProgress);
        winrt::handle progressEvent = Microsoft::Management::Deployment::implementation::AppInstaller::GetProgressEvent();
        if (progressEvent)
        {
            ::SetEvent(progressEvent.get());
        }
    }

    Windows::Foundation::IAsyncAction ExecuteInstallAsync(::AppInstaller::CLI::Execution::Context& context, std::unique_ptr<::AppInstaller::CLI::Command>& command)
    {
        co_await winrt::resume_background();
        ::AppInstaller::CLI::Execute(context, command);
        InstallProgress progress{ AppInstallProgressState::Finished, 0, 0, 0 };
        Microsoft::Management::Deployment::implementation::AppInstaller::SetProgressValue(progress);
        winrt::handle progressEvent = Microsoft::Management::Deployment::implementation::AppInstaller::GetProgressEvent();
        ::SetEvent(progressEvent.get());
    } 
    winrt::handle duplicate(winrt::handle const& other, DWORD access)
    {
        winrt::handle result;
        if (other)
        {
            winrt::check_bool(::DuplicateHandle(::GetCurrentProcess(),
                other.get(), ::GetCurrentProcess(), result.put(), access, FALSE, 0));
        }
        return result;
    }
    void AppInstaller::SetProgressValue(Microsoft::Management::Deployment::InstallProgress progress)
    {
        g_progressValue = progress;
    }
    winrt::handle AppInstaller::GetProgressEvent()
    {
        return duplicate(g_event, 2);
    }
    winrt::handle make_auto_reset_event(bool initialState = false)
    {
        winrt::handle event{ ::CreateEvent(nullptr, false, initialState, nullptr) };
        winrt::check_bool(static_cast<bool>(event));
        return event;
    }
    Windows::Foundation::IAsyncOperationWithProgress<Microsoft::Management::Deployment::InstallResult, Microsoft::Management::Deployment::InstallProgress> AppInstaller::InstallPackageAsync(Microsoft::Management::Deployment::InstallOptions options)
    {
        auto report_progress{ co_await winrt::get_progress_token() };
        auto cancellationToken{ co_await winrt::get_cancellation_token() };
        if (!g_event)
        {
            g_event = make_auto_reset_event();
        }

        Microsoft::Management::Deployment::PackageVersionId versionId{ options.PackageVersionId() };
        Microsoft::Management::Deployment::PackageVersionInfo packageVersionInfo{ nullptr };
        if (versionId)
        {
            packageVersionInfo = options.CatalogPackage().GetAvailableVersion(versionId);
        }
        else
        {
            packageVersionInfo = options.CatalogPackage().LatestAvailableVersion();
        }
        Microsoft::Management::Deployment::AppCatalog catalog = packageVersionInfo.AppCatalog();
        co_await catalog.OpenAsync();

        ::AppInstaller::COMContext context;
        context.SetProgressCallbackFunction(ProgressCallback);
        context.EnableCtrlHandler();

        context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Id, ::AppInstaller::Utility::ConvertToUTF8(options.CatalogPackage().Id()));
        context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Version, ::AppInstaller::Utility::ConvertToUTF8(packageVersionInfo.Version()));
        context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Channel, ::AppInstaller::Utility::ConvertToUTF8(packageVersionInfo.Channel()));
        context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Source, ::AppInstaller::Utility::ConvertToUTF8(catalog.Info().Name()));
        context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Exact);

        ::AppInstaller::CLI::RootCommand rootCommand;
        std::unique_ptr<::AppInstaller::CLI::Command> command = std::make_unique<::AppInstaller::CLI::InstallCommand>(rootCommand.Name());
        Windows::Foundation::IAsyncAction executeOperation = ExecuteInstallAsync(context, command);

        co_await winrt::resume_on_signal(g_event.get(), 2000ms);
        while (true)
        {
            report_progress(g_progressValue);
            if (g_progressValue.State == AppInstallProgressState::Finished)
            {
                break;
            }
            if (cancellationToken())
            {
                context.Terminate(APPINSTALLER_CLI_ERROR_CTRL_SIGNAL_RECEIVED);
            }
            co_await winrt::resume_on_signal(g_event.get(), 2000ms);
        }
        co_await executeOperation;
        g_event.close();

        winrt::Microsoft::Management::Deployment::InstallResult installResult{ nullptr };
        installResult = winrt::make<winrt::Microsoft::Management::Deployment::implementation::InstallResult>(options.CorrelationData(), false);
        co_return installResult;
    }
    Windows::Foundation::IAsyncOperationWithProgress<Microsoft::Management::Deployment::InstallResult, Microsoft::Management::Deployment::InstallProgress> AppInstaller::GetInstallProgress(Microsoft::Management::Deployment::CatalogPackage package)
    {
        throw hresult_not_implemented();
    }
}
