#include "pch.h"
#include "Public/AppInstallerCLICore.h"
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



using namespace std::literals::chrono_literals;

namespace winrt::Microsoft::Management::Deployment::implementation
{
    Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::AppCatalog> AppInstaller::GetAppCatalogs()
    {
        throw hresult_not_implemented();
    }
    Microsoft::Management::Deployment::AppCatalog AppInstaller::GetAppCatalog(Microsoft::Management::Deployment::PredefinedAppCatalog const& predefinedAppCatalog)
    {
        winrt::Microsoft::Management::Deployment::AppCatalog appCatalog{ nullptr };
        appCatalog = winrt::make<winrt::Microsoft::Management::Deployment::implementation::AppCatalog>(L"winget");
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
        throw hresult_not_implemented();
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
        auto report_progress = co_await winrt::get_progress_token();
        if (!g_event)
        {
            g_event = make_auto_reset_event();
        }

        // Enable all logging for this phase; we will update once we have the arguments
        ::AppInstaller::Logging::Log().EnableChannel(::AppInstaller::Logging::Channel::All);
        ::AppInstaller::Logging::Log().SetLevel(::AppInstaller::Logging::Level::Verbose);
        ::AppInstaller::Logging::AddFileLogger();
        ::AppInstaller::Logging::EnableWilFailureTelemetry();

        ::AppInstaller::Logging::Telemetry().LogStartup();

        // Initiate the background cleanup of the log file location.
        ::AppInstaller::Logging::BeginLogFileCleanup();

        ::AppInstaller::COMContext context{ std::cout, std::cin };

        context.SetProgressCallbackFunction(ProgressCallback);
        context.EnableCtrlHandler();

        context << ::AppInstaller::CLI::Workflow::ReportExecutionStage(::AppInstaller::CLI::Workflow::ExecutionStage::ParseArgs);

        // Convert incoming wide char args to UTF8
        std::vector<std::string> utf8Args;
        utf8Args.emplace_back(::AppInstaller::Utility::ConvertToUTF8(L"install"));
        utf8Args.emplace_back(::AppInstaller::Utility::ConvertToUTF8(L"--id"));
        utf8Args.emplace_back(::AppInstaller::Utility::ConvertToUTF8(options.CatalogPackage().Id()));
        ::AppInstaller::CLI::Invocation invocation{ std::move(utf8Args) };

        // The root command is our fallback in the event of very bad or very little input
        std::unique_ptr<::AppInstaller::CLI::Command> command = std::make_unique<::AppInstaller::CLI::RootCommand>();

        try
        {
            std::unique_ptr<::AppInstaller::CLI::Command> subCommand = command->FindSubCommand(invocation);
            while (subCommand)
            {
                command = std::move(subCommand);
                subCommand = command->FindSubCommand(invocation);
            }
            ::AppInstaller::Logging::Telemetry().LogCommand(command->FullName());

            
            command->ParseArguments(invocation, context.Args);

            // Change logging level to Info if Verbose not requested
            //if (!context.Args.Contains(::AppInstaller::CLI::Execution::Args::Type::VerboseLogs))
            //{
                ::AppInstaller::Logging::Log().SetLevel(::AppInstaller::Logging::Level::Info);
            //}

            context.UpdateForArgs();

            command->ValidateArguments(context.Args);
        }
        // Exceptions specific to parsing the arguments of a command
        catch (const ::AppInstaller::CLI::CommandException& ce)
        {
            //command->OutputHelp(context.Reporter, &ce);
            //AICLI_LOG(CLI, Error, << "Error encountered parsing command line: " << ce.Message());
            throw APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS;
        }

        auto executeOperation = ExecuteInstallAsync(context, command);

        co_await winrt::resume_on_signal(g_event.get(), 2000ms);
        while (true)
        {
            report_progress(g_progressValue);
            if (g_progressValue.State == AppInstallProgressState::Finished)
            {
                break;
            }
            co_await winrt::resume_on_signal(g_event.get(), 2000ms);
        }
        co_await executeOperation;
        g_event.close();

        winrt::Microsoft::Management::Deployment::InstallResult installResult{ nullptr };
        installResult = winrt::make<winrt::Microsoft::Management::Deployment::implementation::InstallResult>(options.AdditionalTelemetryArguments(), false);
        co_return installResult;
    }
    Windows::Foundation::IAsyncOperationWithProgress<Microsoft::Management::Deployment::InstallResult, Microsoft::Management::Deployment::InstallProgress> AppInstaller::GetInstallProgress(Microsoft::Management::Deployment::CatalogPackage package)
    {
        throw hresult_not_implemented();
    }
}
