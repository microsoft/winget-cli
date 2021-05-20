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
#include "AppCatalogInfo.h"
#include "AppCatalogReference.h"
#include "PackageVersionInfo.h"
#include "PackageVersionId.h"
#include <wil\cppwinrt_wrl.h>

using namespace std::literals::chrono_literals;

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::AppCatalogReference> AppInstaller::GetAppCatalogs()
    {
        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::AppCatalogReference> catalogs{ winrt::single_threaded_vector<Microsoft::Management::Deployment::AppCatalogReference>() };
        std::vector<::AppInstaller::Repository::SourceDetails> sources = ::AppInstaller::Repository::GetSources();
        for (uint32_t i = 0; i < sources.size(); i++)
        {
            auto appCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::AppCatalogInfo>>();
            appCatalogInfo->Initialize(sources.at(i));
            auto appCatalogImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::AppCatalogReference>>();
            appCatalogImpl->Initialize(*appCatalogInfo);
            catalogs.Append(*appCatalogImpl);
        }
        return catalogs.GetView();
    }
    winrt::Microsoft::Management::Deployment::AppCatalogReference AppInstaller::GetPredefinedAppCatalog(winrt::Microsoft::Management::Deployment::PredefinedAppCatalog const& predefinedAppCatalog)
    {
        return GetAppCatalogById(L"winget");
    }
    winrt::Microsoft::Management::Deployment::AppCatalogReference AppInstaller::GetLocalAppCatalog(winrt::Microsoft::Management::Deployment::LocalAppCatalog const& localAppCatalog)
    {
        ::AppInstaller::Repository::SourceDetails details;
        details.Origin = ::AppInstaller::Repository::SourceOrigin::Predefined;
        details.Type = ::AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Type();
        details.Arg = ::AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::FilterToString(::AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Filter::None);
        auto appCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::AppCatalogInfo>>();
        appCatalogInfo->Initialize(details);
        auto appCatalogImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::AppCatalogReference>>();
        appCatalogImpl->Initialize(localAppCatalog, *appCatalogInfo);
        return *appCatalogImpl;
    }
    winrt::Microsoft::Management::Deployment::AppCatalogReference AppInstaller::GetAppCatalogById(hstring const& catalogId)
    {
        std::optional<::AppInstaller::Repository::SourceDetails> source = ::AppInstaller::Repository::GetSource(winrt::to_string(catalogId));
        if (source.has_value())
        {
            auto appCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::AppCatalogInfo>>();
            appCatalogInfo->Initialize(source.value());
            auto appCatalogImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::AppCatalogReference>>();
            appCatalogImpl->Initialize(*appCatalogInfo);
            return *appCatalogImpl;
        }
        else
        {
            return nullptr;
        }
    }
    winrt::Microsoft::Management::Deployment::AppCatalogReference AppInstaller::CreateCompositeAppCatalog(winrt::Microsoft::Management::Deployment::CreateCompositeAppCatalogOptions const& options)
    {
        if (options.Catalogs().Size() == 0)
        {
            throw hresult_invalid_argument();
        }
        if (options.Catalogs().Size() > 2)
        {
            throw hresult_not_implemented();
        }
        bool includeNonLocalCatalog = false;
        for (uint32_t i = 0; i < options.Catalogs().Size(); ++i)
        {
            auto catalog = options.Catalogs().GetAt(i);
            if (catalog.IsComposite())
            {
                throw hresult_invalid_argument();
            }
            if (winrt::to_string(catalog.Info().Name()).compare(::AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Type()) == 0)
            {
                //includeInstalledCatalog = true;
            }
            else
            {
                includeNonLocalCatalog = true;
            }
        }
        if (!includeNonLocalCatalog)
        {
            throw hresult_not_implemented();
        }
        auto appCatalogImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::AppCatalogReference>>();
        appCatalogImpl->Initialize(options);
        return *appCatalogImpl;
    }

    Windows::Foundation::IAsyncAction ExecuteInstallAsync(::AppInstaller::CLI::Execution::Context& context, std::unique_ptr<::AppInstaller::CLI::Command>& command)
    {
        co_await winrt::resume_background();
        ::AppInstaller::CLI::Execute(context, command);
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> AppInstaller::InstallPackageAsync(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::InstallOptions options)
    {
        auto report_progress{ co_await winrt::get_progress_token() };
        auto cancellationToken{ co_await winrt::get_cancellation_token() };

        Microsoft::Management::Deployment::PackageVersionId versionId{ options.PackageVersionId() };
        Microsoft::Management::Deployment::PackageVersionInfo packageVersionInfo{ nullptr };
        if (versionId)
        {
            packageVersionInfo = package.GetAvailableVersion(versionId);
        }
        else
        {
            packageVersionInfo = package.LatestAvailableVersion();
        }
        Microsoft::Management::Deployment::AppCatalogReference catalogRef = packageVersionInfo.AppCatalogReference();
        Microsoft::Management::Deployment::ConnectResult connectResult = catalogRef.Connect();
        Microsoft::Management::Deployment::AppCatalog catalog = connectResult.AppCatalog();


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
                double downloadPercentage = 0;
                double installPercentage = 0;
                uint64_t downloadBytesDownloaded = 0;
                uint64_t downloadBytesRequired = 0;
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
                            downloadPercentage = downloadBytesDownloaded / downloadBytesRequired;
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
                        installPercentage = current / maximum;
                    }
                    break;
                case ::AppInstaller::CLI::Workflow::ExecutionStage::PostExecution:
                    progressState = AppInstallProgressState::PostInstall;
                    downloadPercentage = 100;
                    installPercentage = 100;
                    break;
                }
                winrt::Microsoft::Management::Deployment::InstallProgress contextProgress{ progressState, downloadBytesDownloaded, downloadBytesRequired, downloadPercentage, installPercentage };
                report_progress(contextProgress);
                return; 
            }
        );
        context.EnableCtrlHandler();

        context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Id, ::AppInstaller::Utility::ConvertToUTF8(package.Id()));
        context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Version, ::AppInstaller::Utility::ConvertToUTF8(packageVersionInfo.Version()));
        context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Channel, ::AppInstaller::Utility::ConvertToUTF8(packageVersionInfo.Channel()));
        context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Source, ::AppInstaller::Utility::ConvertToUTF8(catalog.Info().Name()));
        context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Exact);
        if (!options.LogOutputPath().empty())
        {
            context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Log, ::AppInstaller::Utility::ConvertToUTF8(options.LogOutputPath()));
            context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::VerboseLogs);
        }
        if (options.AllowHashMismatch())
        {
            context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::HashOverride);
        }

        if (options.AppInstallScope() == AppInstallScope::Machine)
        {
            context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::InstallScope, ScopeToString(::AppInstaller::Manifest::ScopeEnum::Machine));
        }
        else
        {
            context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::InstallScope, ScopeToString(::AppInstaller::Manifest::ScopeEnum::User));
        }

        if (options.AppInstallMode() == AppInstallMode::Interactive)
        {
            context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Interactive);
        }
        else if (options.AppInstallMode() == AppInstallMode::Silent)
        {
            context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Silent);
        }

        if (!options.PreferredInstallLocation().empty())
        {
            context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::InstallLocation, ::AppInstaller::Utility::ConvertToUTF8(options.PreferredInstallLocation()));
        }

        if (!options.ReplacementInstallerArguments().empty())
        {
            context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Override, ::AppInstaller::Utility::ConvertToUTF8(options.ReplacementInstallerArguments()));
        }    

        // TODO: AdditionalAppCatalogArguments is not currently supported by the underlying implementation.
        // TODO: CorrelationData is not currently supported by the underlying implementation.

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
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> AppInstaller::GetInstallProgress(winrt::Microsoft::Management::Deployment::CatalogPackage package)
    {
        throw hresult_not_implemented();
    }
    CoCreatableCppWinRtClass(AppInstaller);
}
