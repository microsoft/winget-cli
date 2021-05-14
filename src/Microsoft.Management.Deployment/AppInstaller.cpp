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
#include <wil\cppwinrt_wrl.h>

using namespace std::literals::chrono_literals;

namespace winrt::Microsoft::Management::Deployment::implementation
{
    Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::AppCatalog> AppInstaller::GetAppCatalogs()
    {
        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::AppCatalog> catalogs{ winrt::single_threaded_vector<Microsoft::Management::Deployment::AppCatalog>() };
        std::vector<::AppInstaller::Repository::SourceDetails> sources = ::AppInstaller::Repository::GetSources();
        for (uint32_t i = 0; i < sources.size(); i++)
        {
            auto appCatalogImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::AppCatalog>>();
            appCatalogImpl->Initialize(winrt::to_hstring(sources.at(i).Name));
            catalogs.Append(*appCatalogImpl);
        }
        return catalogs.GetView();
    }
    Microsoft::Management::Deployment::AppCatalog AppInstaller::GetAppCatalog(Microsoft::Management::Deployment::PredefinedAppCatalog const& predefinedAppCatalog)
    {
        auto appCatalogImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::AppCatalog>>();
        appCatalogImpl->Initialize(predefinedAppCatalog);
        return *appCatalogImpl;
    }
    Microsoft::Management::Deployment::AppCatalog AppInstaller::GetAppCatalogByLocalAppCatalog(Microsoft::Management::Deployment::LocalAppCatalog const& localAppCatalog)
    {
        auto appCatalogImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::AppCatalog>>();
        appCatalogImpl->Initialize(localAppCatalog);
        return *appCatalogImpl;
    }
    Microsoft::Management::Deployment::AppCatalog AppInstaller::GetAppCatalogById(hstring const& catalogId)
    {
        auto appCatalogImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::AppCatalog>>();
        appCatalogImpl->Initialize(catalogId);
        return *appCatalogImpl;
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
        auto appCatalogImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::AppCatalog>>();
        appCatalogImpl->Initialize(options);
        return *appCatalogImpl;
    }

    Windows::Foundation::IAsyncAction ExecuteInstallAsync(::AppInstaller::CLI::Execution::Context& context, std::unique_ptr<::AppInstaller::CLI::Command>& command)
    {
        co_await winrt::resume_background();
        ::AppInstaller::CLI::Execute(context, command);
    }
    Windows::Foundation::IAsyncOperationWithProgress<Microsoft::Management::Deployment::InstallResult, Microsoft::Management::Deployment::InstallProgress> AppInstaller::InstallPackageAsync(Microsoft::Management::Deployment::InstallOptions options)
    {
        auto report_progress{ co_await winrt::get_progress_token() };
        auto cancellationToken{ co_await winrt::get_cancellation_token() };

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
        // TODO: shouldn't be necessary, should already be open if using the same object.
        co_await catalog.OpenAsync();

        InstallProgress queuedProgress{ AppInstallProgressState::Queued, 0, 0, 0 };
        report_progress(queuedProgress);


        ::AppInstaller::COMContext context;
        context.SetProgressCallbackFunction([=](
            ::AppInstaller::ReportType reportType, 
            uint64_t current, 
            uint64_t maximum, 
            ::AppInstaller::ProgressType progressType, 
            ::AppInstaller::CLI::Workflow::ExecutionStage executionPhase)
            { 
                AppInstallProgressState progressState = AppInstallProgressState::Queued;
                float downloadPercentage = 0;
                float installPercentage = 0;
                float downloadBytesDownloaded = 0;
                float downloadBytesRequired = 0;
                switch (executionPhase)
                {
                case ::AppInstaller::CLI::Workflow::ExecutionStage::Initial:
                case ::AppInstaller::CLI::Workflow::ExecutionStage::ParseArgs:
                case ::AppInstaller::CLI::Workflow::ExecutionStage::Discovery:
                    progressState = AppInstallProgressState::Queued;
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
                    progressState = AppInstallProgressState::Installing;
                    downloadPercentage = 100;
                case ::AppInstaller::CLI::Workflow::ExecutionStage::Execution:
                    progressState = AppInstallProgressState::Installing;
                    downloadPercentage = 100;
                    if (progressType == ::AppInstaller::ProgressType::Percent)
                    {
                        installPercentage = current;
                    }
                    break;
                case ::AppInstaller::CLI::Workflow::ExecutionStage::PostExecution:
                    progressState = AppInstallProgressState::PostInstall;
                    downloadPercentage = 100;
                    installPercentage = 100;
                    break;
                }
                InstallProgress contextProgress{ progressState, downloadBytesDownloaded, downloadBytesRequired, downloadPercentage, installPercentage };
                report_progress(contextProgress);
                return; 
            }
        );
        context.EnableCtrlHandler();

        context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Id, ::AppInstaller::Utility::ConvertToUTF8(options.CatalogPackage().Id()));
        context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Version, ::AppInstaller::Utility::ConvertToUTF8(packageVersionInfo.Version()));
        context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Channel, ::AppInstaller::Utility::ConvertToUTF8(packageVersionInfo.Channel()));
        context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Source, ::AppInstaller::Utility::ConvertToUTF8(catalog.Info().Name()));
        context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Exact);
        if (!options.LogOutputPath().empty())
        {
            context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Log, ::AppInstaller::Utility::ConvertToUTF8(options.LogOutputPath()));
            context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::VerboseLogs);
        }

        ::AppInstaller::CLI::RootCommand rootCommand;
        std::unique_ptr<::AppInstaller::CLI::Command> command = std::make_unique<::AppInstaller::CLI::InstallCommand>(rootCommand.Name());
        Windows::Foundation::IAsyncAction executeOperation = ExecuteInstallAsync(context, command);

        cancellationToken.callback([&context]
            {
                context.Terminate(APPINSTALLER_CLI_ERROR_CTRL_SIGNAL_RECEIVED);
            });
        co_await executeOperation;

        // TODO - RebootRequired not yet populated
        auto installResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::InstallResult>>();
        installResult->Initialize(options.CorrelationData(), false);
        co_return *installResult;
    }
    Windows::Foundation::IAsyncOperationWithProgress<Microsoft::Management::Deployment::InstallResult, Microsoft::Management::Deployment::InstallProgress> AppInstaller::GetInstallProgress(Microsoft::Management::Deployment::CatalogPackage package)
    {
        throw hresult_not_implemented();
    }
    CoCreatableCppWinRtClass(AppInstaller);
}
