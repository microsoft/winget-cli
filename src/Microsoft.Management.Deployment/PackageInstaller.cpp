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
#include "Converters.h"
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
            if (sources.at(i).Identifier == "Microsoft.Winget.Source_8wekyb3d8bbwe")
            {
                continue;
            }
            auto packageCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>>();
            packageCatalogInfo->Initialize(sources.at(i));
            auto packageCatalogRef = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference>>();
            packageCatalogRef->Initialize(sources.at(i), *packageCatalogInfo);
            catalogs.Append(*packageCatalogRef);
        }
        return catalogs.GetView();
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageInstaller::GetPredefinedPackageCatalog(winrt::Microsoft::Management::Deployment::PredefinedPackageCatalog const& predefinedPackageCatalog)
    {
        switch (predefinedPackageCatalog)
        {
        case winrt::Microsoft::Management::Deployment::PredefinedPackageCatalog::OpenWindowsCatalog:
            // TODO: Mapping of enum to sources should link directly to definition, not copy string.
            return GetPackageCatalogById(L"Microsoft.Winget.Source_8wekyb3d8bbwe");
        default:
            throw hresult_invalid_argument();
        }
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageInstaller::GetLocalPackageCatalog(winrt::Microsoft::Management::Deployment::LocalPackageCatalog const& localPackageCatalog)
    {
        // InstalledPackages is the only one supported right now, so return early if it's not that.
        if(localPackageCatalog != Microsoft::Management::Deployment::LocalPackageCatalog::InstalledPackages)
        {
            throw hresult_invalid_argument();
        }

        ::AppInstaller::Repository::SourceDetails details;
        details.Origin = ::AppInstaller::Repository::SourceOrigin::Predefined;
        details.Type = ::AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Type();
        details.Arg = ::AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::FilterToString(::AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Filter::None);

        auto packageCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>>();
        packageCatalogInfo->Initialize(details);
        auto packageCatalogImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference>>();
        packageCatalogImpl->Initialize(details, *packageCatalogInfo);
        return *packageCatalogImpl;
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageInstaller::GetPackageCatalogById(hstring const& catalogId)
    {
        std::optional<::AppInstaller::Repository::SourceDetails> source = ::AppInstaller::Repository::GetSourceByIdentifier(winrt::to_string(catalogId));
        // Create the catalog object if the source is found, otherwise return null. Don't throw.
        if (source.has_value())
        {
            auto packageCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>>();
            packageCatalogInfo->Initialize(source.value());
            auto packageCatalogRef = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference>>();
            packageCatalogRef->Initialize(source.value(), *packageCatalogInfo);
            return *packageCatalogRef;
        }
        else
        {
            return nullptr;
        }
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageInstaller::CreateCompositePackageCatalog(winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions const& options)
    {
        if (options.Catalogs().Size() == 0 && !options.LocalPackageCatalog())
        {
            // Can't create a composite catalog with no arguments.
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
            if (winrt::to_string(catalog.Info().Type()).compare(::AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Type()) == 0)
            {
                // Local catalogs are only allowed in the LocalPackageCatalog property
                throw hresult_invalid_argument();
            }
        }
        if (options.LocalPackageCatalog())
        {
            if (winrt::to_string(options.LocalPackageCatalog().Info().Type()).compare(::AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Type()) != 0)
            {
                // Only Local catalogs are allowed in the LocalPackageCatalog property
                throw hresult_invalid_argument();
            }
        }
        else if (options.CompositeSearchBehavior() == CompositeSearchBehavior::LocalCatalogs)
        {
            // No local catalog is specified so CompositeSearchBehavior cannot be LocalCatalogs.
            throw hresult_invalid_argument();
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

        InstallProgress queuedProgress{ PackageInstallProgressState::Queued, 0, 0, 0 };
        report_progress(queuedProgress);

        Microsoft::Management::Deployment::PackageVersionId versionId{ nullptr };
        if (options)
        {
            versionId = options.PackageVersionId();
        }

        // If the version of the package is specified use that, otherwise use the default.
        Microsoft::Management::Deployment::PackageVersionInfo packageVersionInfo{ nullptr };
        if (versionId)
        {
            packageVersionInfo = package.GetPackageVersionInfo(versionId);
        }
        else
        {
            packageVersionInfo = package.DefaultInstallVersion();
        }

        if (!packageVersionInfo)
        {
            // If no package version was found on the catalog then return a failure. This is unexpected, a catalog with no latest version should not be in the catalog.
            HRESULT terminationHR = APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER;
            winrt::Microsoft::Management::Deployment::InstallResultStatus installResultStatus = GetInstallResultStatus(terminationHR);
            auto installResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::InstallResult>>();
            installResult->Initialize(installResultStatus, terminationHR, options.CorrelationData(), false);
            co_return *installResult;
        }

        // Handle the progress from the installer
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
                    break;
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

        // Convert the options to arguments for the installer.
        context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Id, ::AppInstaller::Utility::ConvertToUTF8(package.Id()));
        context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Version, ::AppInstaller::Utility::ConvertToUTF8(packageVersionInfo.Version()));
        context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Channel, ::AppInstaller::Utility::ConvertToUTF8(packageVersionInfo.Channel()));
        context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Source, ::AppInstaller::Utility::ConvertToUTF8(packageVersionInfo.PackageCatalog().Info().Name()));
        context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Exact);
        if (options)
        {
            if (!options.LogOutputPath().empty())
            {
                context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::Log, ::AppInstaller::Utility::ConvertToUTF8(options.LogOutputPath()));
                context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::VerboseLogs);
            }
            if (options.AllowHashMismatch())
            {
                context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::HashOverride);
            }

            // If the PackageInstallScope is anything other than ::Any then set it as a requirement.
            if (options.PackageInstallScope() == PackageInstallScope::System)
            {
                context.Args.AddArg(::AppInstaller::CLI::Execution::Args::Type::InstallScope, ScopeToString(::AppInstaller::Manifest::ScopeEnum::Machine));
            }
            else if (options.PackageInstallScope() == PackageInstallScope::User)
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
        // Wait for the execute operation to finish. 
        // The cancellation of the AsyncOperation triggers Terminate which causes the executeOperation to end.
        co_await executeOperation;

        HRESULT terminationHR = context.GetTerminationHR();
        winrt::Microsoft::Management::Deployment::InstallResultStatus installResultStatus = GetInstallResultStatus(terminationHR);

        // TODO - RebootRequired not yet populated, msi arguments not returned from Execute.
        auto installResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::InstallResult>>();
        installResult->Initialize(installResultStatus, terminationHR, options.CorrelationData(), false);
        co_return *installResult;
    }
    CoCreatableCppWinRtClass(PackageInstaller);
}
