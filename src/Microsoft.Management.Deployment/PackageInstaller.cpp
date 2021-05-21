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
#include "PackageInstaller.h"
#include "PackageInstaller.g.cpp"
#include "InstallResult.h"
#include "PackageCatalogInfo.h"
#include "PackageCatalogReference.h"
#include "PackageVersionInfo.h"
#include "PackageVersionId.h"
#include <wil\cppwinrt_wrl.h>

using namespace std::literals::chrono_literals;

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::PackageCatalogReference> PackageInstaller::GetUserPackageCatalogs()
    {
        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::PackageCatalogReference> catalogs{ winrt::single_threaded_vector<Microsoft::Management::Deployment::PackageCatalogReference>() };
        std::vector<::AppInstaller::Repository::SourceDetails> sources = ::AppInstaller::Repository::GetSources();
        for (uint32_t i = 0; i < sources.size(); i++)
        {
            auto packageCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>>();
            packageCatalogInfo->Initialize(sources.at(i));
            auto packageCatalogImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference>>();
            packageCatalogImpl->Initialize(*packageCatalogInfo);
            catalogs.Append(*packageCatalogImpl);
        }
        return catalogs.GetView();
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageInstaller::GetPredefinedPackageCatalog(winrt::Microsoft::Management::Deployment::PredefinedPackageCatalog const& predefinedPackageCatalog)
    {
        return GetPackageCatalogById(L"winget");
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageInstaller::GetLocalPackageCatalog(winrt::Microsoft::Management::Deployment::LocalPackageCatalog const& localPackageCatalog)
    {
        ::AppInstaller::Repository::SourceDetails details;
        details.Origin = ::AppInstaller::Repository::SourceOrigin::Predefined;
        details.Type = ::AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Type();
        details.Arg = ::AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::FilterToString(::AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Filter::None);
        auto packageCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>>();
        packageCatalogInfo->Initialize(details);
        auto packageCatalogImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference>>();
        packageCatalogImpl->Initialize(localPackageCatalog, *packageCatalogInfo);
        return *packageCatalogImpl;
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageInstaller::GetPackageCatalogById(hstring const& catalogId)
    {
        std::optional<::AppInstaller::Repository::SourceDetails> source = ::AppInstaller::Repository::GetSource(winrt::to_string(catalogId));
        if (source.has_value())
        {
            auto packageCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>>();
            packageCatalogInfo->Initialize(source.value());
            auto packageCatalogImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference>>();
            packageCatalogImpl->Initialize(*packageCatalogInfo);
            return *packageCatalogImpl;
        }
        else
        {
            return nullptr;
        }
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageInstaller::CreateCompositePackageCatalog(winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions const& options)
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
        auto packageCatalogImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference>>();
        packageCatalogImpl->Initialize(options);
        return *packageCatalogImpl;
    }

    Windows::Foundation::IAsyncAction ExecuteInstallAsync(::AppInstaller::CLI::Execution::Context& context, std::unique_ptr<::AppInstaller::CLI::Command>& command)
    {
        co_await winrt::resume_background();
        ::AppInstaller::CLI::Execute(context, command);
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> PackageInstaller::InstallPackageAsync(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::InstallOptions options)
    {
        auto report_progress{ co_await winrt::get_progress_token() };
        auto cancellationToken{ co_await winrt::get_cancellation_token() };

        Microsoft::Management::Deployment::PackageVersionId versionId{ options.PackageVersionId() };
        Microsoft::Management::Deployment::PackageVersionInfo packageVersionInfo{ nullptr };
        if (versionId)
        {
            packageVersionInfo = package.GetPackageVersionInfo(versionId);
        }
        else
        {
            packageVersionInfo = package.DefaultInstallVersion();
        }
        Microsoft::Management::Deployment::PackageCatalogReference catalogRef = packageVersionInfo.PackageCatalogReference();
        Microsoft::Management::Deployment::ConnectResult connectResult = catalogRef.Connect();
        Microsoft::Management::Deployment::PackageCatalog catalog = connectResult.PackageCatalog();


        InstallProgress queuedProgress{ PackageInstallProgressState::Queued, 0, 0, 0 };
        report_progress(queuedProgress);

        ::AppInstaller::COMContext context;
        context.SetProgressCallbackFunction([=](
            ::AppInstaller::ReportType reportType, 
            uint64_t current, 
            uint64_t maximum, 
            ::AppInstaller::ProgressType progressType, 
            ::AppInstaller::CLI::Workflow::ExecutionStage executionPhase)
            { 
                PackageInstallProgressState progressState = PackageInstallProgressState::Queued;
                double downloadPercentage = 0;
                double installPercentage = 0;
                uint64_t downloadBytesDownloaded = 0;
                uint64_t downloadBytesRequired = 0;
                switch (executionPhase)
                {
                case ::AppInstaller::CLI::Workflow::ExecutionStage::Initial:
                case ::AppInstaller::CLI::Workflow::ExecutionStage::ParseArgs:
                case ::AppInstaller::CLI::Workflow::ExecutionStage::Discovery:
                    progressState = PackageInstallProgressState::Queued;
                    break;
                case ::AppInstaller::CLI::Workflow::ExecutionStage::Download:
                    progressState = PackageInstallProgressState::Downloading;
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
                    progressState = PackageInstallProgressState::Installing;
                    downloadPercentage = 100;
                case ::AppInstaller::CLI::Workflow::ExecutionStage::Execution:
                    progressState = PackageInstallProgressState::Installing;
                    downloadPercentage = 100;
                    if (progressType == ::AppInstaller::ProgressType::Percent)
                    {
                        installPercentage = current / maximum;
                    }
                    break;
                case ::AppInstaller::CLI::Workflow::ExecutionStage::PostExecution:
                    progressState = PackageInstallProgressState::PostInstall;
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

        if (options.PackageInstallScope() == PackageInstallScope::System)
        {
            context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::InstallScope, ScopeToString(::AppInstaller::Manifest::ScopeEnum::Machine));
        }
        else
        {
            context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::InstallScope, ScopeToString(::AppInstaller::Manifest::ScopeEnum::User));
        }

        if (options.PackageInstallMode() == PackageInstallMode::Interactive)
        {
            context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Interactive);
        }
        else if (options.PackageInstallMode() == PackageInstallMode::Silent)
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

        // TODO: AdditionalPackageCatalogArguments is not currently supported by the underlying implementation.
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
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> PackageInstaller::GetInstallProgress(winrt::Microsoft::Management::Deployment::CatalogPackage package)
    {
        throw hresult_not_implemented();
    }
    CoCreatableCppWinRtClass(PackageInstaller);
}
