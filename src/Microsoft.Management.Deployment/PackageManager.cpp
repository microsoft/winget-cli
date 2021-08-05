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

using namespace std::literals::chrono_literals;

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
        // InstalledPackages is the only one supported right now, so return early if it's not that.
        if (localPackageCatalog != Microsoft::Management::Deployment::LocalPackageCatalog::InstalledPackages)
        {
            throw hresult_invalid_argument();
        }
        ::AppInstaller::Repository::SourceDetails sourceDetails = GetPredefinedSourceDetails(::AppInstaller::Repository::PredefinedSource::Installed);
        auto packageCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>>();
        packageCatalogInfo->Initialize(sourceDetails);
        auto packageCatalogImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference>>();
        packageCatalogImpl->Initialize(*packageCatalogInfo);
        return *packageCatalogImpl;
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
    void AddPackageManifestToContext(winrt::Microsoft::Management::Deployment::PackageVersionInfo packageVersionInfo, ::AppInstaller::CLI::Execution::Context& context)
    {
        winrt::Microsoft::Management::Deployment::implementation::PackageVersionInfo* packageVersionInfoImpl = get_self<winrt::Microsoft::Management::Deployment::implementation::PackageVersionInfo>(packageVersionInfo);
        std::shared_ptr<::AppInstaller::Repository::IPackageVersion> internalPackageVersion = packageVersionInfoImpl->GetRepositoryPackageVersion();
        ::AppInstaller::Manifest::Manifest manifest = internalPackageVersion->GetManifest();

        std::string targetLocale;
        if (context.Args.Contains(::AppInstaller::CLI::Execution::Args::Type::Locale))
        {
            targetLocale = context.Args.GetArg(::AppInstaller::CLI::Execution::Args::Type::Locale);
        }
        manifest.ApplyLocale(targetLocale);

        context.Add<::AppInstaller::CLI::Execution::Data::Manifest>(std::move(manifest));
        context.Add<::AppInstaller::CLI::Execution::Data::PackageVersion>(std::move(internalPackageVersion));

        ::AppInstaller::Logging::Telemetry().LogManifestFields(manifest.Id, manifest.DefaultLocalization.Get<::AppInstaller::Manifest::Localization::PackageName>(), manifest.Version);
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

    Windows::Foundation::IAsyncOperation<winrt::hresult> ExecuteInstallAsync(::AppInstaller::CLI::Execution::Context& context, std::unique_ptr<::AppInstaller::CLI::Command>& command)
    {
        co_await winrt::resume_background();
        winrt::hresult result = ::AppInstaller::CLI::Execute(context, command);
        return result;
    }
    winrt::Microsoft::Management::Deployment::InstallResult GetInstallResult(::AppInstaller::CLI::Workflow::ExecutionStage executionStage, winrt::hresult terminationHR, winrt::hstring correlationData, bool rebootRequired)
    {
        winrt::Microsoft::Management::Deployment::InstallResultStatus installResultStatus = GetInstallResultStatus(executionStage, terminationHR);
        auto installResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::InstallResult>>();
        installResult->Initialize(installResultStatus, terminationHR, correlationData, rebootRequired);
        return *installResult;
    }
    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> PackageManager::InstallPackageAsync(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::InstallOptions options)
    {
        winrt::hresult terminationHR = S_OK;
        ::AppInstaller::CLI::Workflow::ExecutionStage executionStage = ::AppInstaller::CLI::Workflow::ExecutionStage::Initial;

        try
        {
            std::optional<DWORD> callerProcessId = GetCallerProcessId();
            if (!callerProcessId.has_value())
            {
                co_return GetInstallResult(executionStage, E_ACCESSDENIED, options.CorrelationData(), false);
            }
            if (FAILED(terminationHR = EnsureProcessHasCapability(Capability::PackageManagement, callerProcessId.value())))
            {
                co_return GetInstallResult(executionStage, terminationHR, options.CorrelationData(), false);
            }
            std::wstring callerProcessInfoString = TryGetCallerProcessInfo(callerProcessId.value());

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
                co_return GetInstallResult(executionStage, APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER, options.CorrelationData(), false);
            }

            // Handle the progress from the installer
            ::AppInstaller::COMContext context;

            context.SetLoggerContext(options.CorrelationData(), ::AppInstaller::Utility::ConvertToUTF8(callerProcessInfoString));

            // Convert the options to arguments for the installer.
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

            AddPackageManifestToContext(packageVersionInfo, context);

            // TODO: AdditionalPackageCatalogArguments is not currently supported by the underlying implementation.
            ::AppInstaller::CLI::RootCommand rootCommand;
            std::unique_ptr<::AppInstaller::CLI::Command> command = std::make_unique<::AppInstaller::CLI::COMInstallCommand>(rootCommand.Name());
            rootCommand.ValidateArguments(context.Args);

            ::AppInstaller::Logging::Telemetry().LogCommand(command->FullName());

            context.SetProgressCallbackFunction([=](
                ::AppInstaller::ReportType reportType,
                uint64_t current,
                uint64_t maximum,
                ::AppInstaller::ProgressType progressType,
                ::AppInstaller::CLI::Workflow::ExecutionStage executionPhase)
                {
                    bool reportProgress = false;
                    PackageInstallProgressState progressState = PackageInstallProgressState::Queued;
                    double downloadProgress = 0;
                    double installProgress = 0;
                    uint64_t downloadBytesDownloaded = 0;
                    uint64_t downloadBytesRequired = 0;
                    switch (executionPhase)
                    {
                    case ::AppInstaller::CLI::Workflow::ExecutionStage::Initial:
                    case ::AppInstaller::CLI::Workflow::ExecutionStage::ParseArgs:
                    case ::AppInstaller::CLI::Workflow::ExecutionStage::Discovery:
                        // We already reported queued progress up front.
                        break;
                    case ::AppInstaller::CLI::Workflow::ExecutionStage::Download:
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
                    case ::AppInstaller::CLI::Workflow::ExecutionStage::PreExecution:
                        // Wait until installer starts to report Installing.
                        break;
                    case ::AppInstaller::CLI::Workflow::ExecutionStage::Execution:
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
                    case ::AppInstaller::CLI::Workflow::ExecutionStage::PostExecution:
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
                        report_progress(contextProgress);
                    }
                    return;
                }
            );
            context.EnableCtrlHandler();

            Windows::Foundation::IAsyncOperation<winrt::hresult> executeOperation = ExecuteInstallAsync(context, command);

            cancellationToken.callback([&context]
                {
                    context.Cancel(false, true);
                });
            // Wait for the execute operation to finish. 
            // The cancellation of the AsyncOperation triggers Cancel which causes the executeOperation to end.
            terminationHR = co_await executeOperation;
            executionStage = context.GetExecutionStage();

        }
        WINGET_CATCH_STORE(terminationHR);

        // TODO - RebootRequired not yet populated, msi arguments not returned from Execute.
        co_return GetInstallResult(executionStage, terminationHR, options.CorrelationData(), false);
    }
    CoCreatableCppWinRtClassWithCLSID(PackageManager, 1, &PackageManagerCLSID1);
    CoCreatableCppWinRtClassWithCLSID(PackageManager, 2, &PackageManagerCLSID2);
}
