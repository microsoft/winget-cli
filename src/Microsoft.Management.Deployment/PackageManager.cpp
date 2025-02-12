// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerCLICore.h"
#include "Microsoft/PredefinedInstalledSourceFactory.h"
#include "Commands/RootCommand.h"
#include "ComContext.h"
#include "ExecutionContext.h"
#include "Workflows/WorkflowBase.h"
#include <winget/Authentication.h>
#include <winget/UserSettings.h>
#include <winget/Manifest.h>
#include "Commands/COMCommand.h"
#include <AppInstallerArchitecture.h>
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
#include "CatalogPackage.h"
#include "DownloadResult.h"
#include "InstallResult.h"
#include "UninstallResult.h"
#include "RepairResult.h"
#include "PackageCatalogInfo.h"
#include "PackageCatalogReference.h"
#include "PackageVersionInfo.h"
#include "PackageVersionId.h"
#include "AddPackageCatalogResult.h"
#include "RemovePackageCatalogResult.h"
#include "Converters.h"
#include "Helpers.h"
#include "ContextOrchestrator.h"
#include "AppInstallerRuntime.h"
#include <optional>
#include <PackageCatalogProgress.h>

using namespace std::literals::chrono_literals;
using namespace ::AppInstaller::CLI;
using namespace ::AppInstaller::CLI::Execution;

namespace winrt::Microsoft::Management::Deployment::implementation
{
    namespace
    {
        void LogStartupIfApplicable()
        {
            static std::once_flag logStartupOnceFlag;
            std::call_once(logStartupOnceFlag,
                [&]()
                {
                    ::AppInstaller::Logging::Telemetry().SetCaller(GetCallerName());
                    ::AppInstaller::Logging::Telemetry().LogStartup(true);
                });
        }

        winrt::Microsoft::Management::Deployment::AddPackageCatalogResult GetAddPackageCatalogResult(winrt::hresult terminationStatus)
        {
            winrt::Microsoft::Management::Deployment::AddPackageCatalogStatus status = GetPackageCatalogOperationStatus<AddPackageCatalogStatus>(terminationStatus);
            auto addPackageCatalogResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::AddPackageCatalogResult>>();
            addPackageCatalogResult->Initialize(status, terminationStatus);
            return *addPackageCatalogResult;
        }

        void CheckForDuplicateSource(const std::string& name, const std::string& type, const std::string& sourceUri)
        {
            auto sourceList = ::AppInstaller::Repository::Source::GetCurrentSources();

            std::string sourceType = type;

            // [NOTE:] If the source type is not specified, the default source type will be used for validation.In cases where the source type is empty,
            // it remains unassigned until the add operation, at which point it is assigned.Without this default assignment, an empty string could be
            // compared to the default type, potentially allowing different source names with the same URI to be seen as unique.
            // To avoid this, assign the default source type prior to comparison.
            if (sourceType.empty())
            {
                // This method of obtaining the default source type is slightly expensive as it requires creating a SourceFactory object
                // and fetching the type name.Nonetheless, it future-proofs the code against any changes in the SourceFactory's default type.
                sourceType = ::AppInstaller::Repository::Source::GetDefaultSourceType();
            }

            for (const auto& source : sourceList)
            {
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS, ::AppInstaller::Utility::ICUCaseInsensitiveEquals(source.Name, name));

                bool sourceUriAlreadyExists = !source.Arg.empty() && source.Arg == sourceUri && source.Type == sourceType;
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_ARG_ALREADY_EXISTS, sourceUriAlreadyExists);
            }
        }

        ::AppInstaller::Repository::Source CreateSourceFromOptions(const winrt::Microsoft::Management::Deployment::AddPackageCatalogOptions& options)
        {
            std::string name = winrt::to_string(options.Name());
            std::string type = winrt::to_string(options.Type());
            std::string sourceUri = winrt::to_string(options.SourceUri());

            AppInstaller::Repository::SourceTrustLevel trustLevel = AppInstaller::Repository::SourceTrustLevel::None;
            if (options.TrustLevel() == winrt::Microsoft::Management::Deployment::PackageCatalogTrustLevel::Trusted)
            {
                trustLevel = AppInstaller::Repository::SourceTrustLevel::Trusted;
            }

            CheckForDuplicateSource(name, type, sourceUri);

            ::AppInstaller::Repository::Source source = ::AppInstaller::Repository::Source{ name, sourceUri, type, trustLevel, options.Explicit() };

            std::string customHeader = winrt::to_string(options.CustomHeader());
            if (!customHeader.empty())
            {
                source.SetCustomHeader(customHeader);
            }

            auto sourceInfo = source.GetInformation();

            if (sourceInfo.Authentication.Type == ::AppInstaller::Authentication::AuthenticationType::Unknown)
            {
                THROW_HR(APPINSTALLER_CLI_ERROR_AUTHENTICATION_TYPE_NOT_SUPPORTED);
            }

            return source;
        }

        winrt::Microsoft::Management::Deployment::RemovePackageCatalogResult GetRemovePackageCatalogResult(winrt::hresult terminationStatus)
        {
            winrt::Microsoft::Management::Deployment::RemovePackageCatalogStatus status = GetPackageCatalogOperationStatus<RemovePackageCatalogStatus>(terminationStatus);
            auto removeResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::RemovePackageCatalogResult>>();
            removeResult->Initialize(status, terminationStatus);
            return *removeResult;
        }

        std::optional<::AppInstaller::Repository::SourceDetails> GetMatchingSource(const std::string& name)
        {
            auto sourceList = ::AppInstaller::Repository::Source::GetCurrentSources();

            for (const auto& source : sourceList)
            {
                if (::AppInstaller::Utility::ICUCaseInsensitiveEquals(source.Name, name))
                {
                    return source; // Return the first matching source
                }
            }

            return std::nullopt; // Return std::nullopt if no matching source is found
        }
    }

    winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::PackageCatalogReference> PackageManager::GetPackageCatalogs()
    {
        LogStartupIfApplicable();
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
        LogStartupIfApplicable();
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
        LogStartupIfApplicable();
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
        LogStartupIfApplicable();
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

    void AddInstalledVersionToContext(winrt::Microsoft::Management::Deployment::PackageVersionInfo installedVersionInfo, ::AppInstaller::CLI::Execution::Context* context)
    {
        winrt::Microsoft::Management::Deployment::implementation::PackageVersionInfo* installedVersionInfoImpl = get_self<winrt::Microsoft::Management::Deployment::implementation::PackageVersionInfo>(installedVersionInfo);
        std::shared_ptr<::AppInstaller::Repository::IPackageVersion> internalInstalledVersion = installedVersionInfoImpl->GetRepositoryPackageVersion();
        context->Add<AppInstaller::CLI::Execution::Data::InstalledPackageVersion>(internalInstalledVersion);
    }

    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageManager::CreateCompositePackageCatalog(winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions const& options)
    {
        LogStartupIfApplicable();
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

    winrt::Microsoft::Management::Deployment::InstallResult GetInstallResult(::Workflow::ExecutionStage executionStage, winrt::hresult terminationHR, uint32_t installerError, winrt::hstring correlationData, bool rebootRequired)
    {
        winrt::Microsoft::Management::Deployment::InstallResultStatus installResultStatus = GetOperationResultStatus<InstallResultStatus>(executionStage, terminationHR);
        auto installResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::InstallResult>>();
        installResult->Initialize(installResultStatus, terminationHR, installerError, correlationData, rebootRequired);
        return *installResult;
    }

    winrt::Microsoft::Management::Deployment::UninstallResult GetUninstallResult(::Workflow::ExecutionStage executionStage, winrt::hresult terminationHR, uint32_t uninstallerError, winrt::hstring correlationData, bool rebootRequired)
    {
        winrt::Microsoft::Management::Deployment::UninstallResultStatus uninstallResultStatus = GetOperationResultStatus<UninstallResultStatus>(executionStage, terminationHR);
        auto uninstallResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::UninstallResult>>();
        uninstallResult->Initialize(uninstallResultStatus, terminationHR, uninstallerError, correlationData, rebootRequired);
        return *uninstallResult;
    }

    winrt::Microsoft::Management::Deployment::DownloadResult GetDownloadResult(::Workflow::ExecutionStage executionStage, winrt::hresult terminationHR, winrt::hstring correlationData)
    {
        winrt::Microsoft::Management::Deployment::DownloadResultStatus downloadResultStatus = GetOperationResultStatus<DownloadResultStatus>(executionStage, terminationHR);
        auto downloadResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::DownloadResult>>();
        downloadResult->Initialize(downloadResultStatus, terminationHR, correlationData);
        return *downloadResult;
    }

    winrt::Microsoft::Management::Deployment::RepairResult GetRepairResult(::Workflow::ExecutionStage executionStage, winrt::hresult terminationHR, uint32_t repairError, winrt::hstring correlationData, bool rebootRequired)
    {
        winrt::Microsoft::Management::Deployment::RepairResultStatus repairResultStatus = GetOperationResultStatus<RepairResultStatus>(executionStage, terminationHR);
        auto repairResult = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::RepairResult>>();
        repairResult->Initialize(repairResultStatus, terminationHR, repairError, correlationData, rebootRequired);
        return *repairResult;
    }

    template <typename TResult>
    TResult GetOperationResult(::Workflow::ExecutionStage executionStage, winrt::hresult terminationHR, uint32_t operationError, winrt::hstring correlationData, bool rebootRequired)
    {
        if constexpr (std::is_same_v<TResult, winrt::Microsoft::Management::Deployment::InstallResult>)
        {
            return GetInstallResult(executionStage, terminationHR, operationError, correlationData, rebootRequired);
        }
        else if constexpr (std::is_same_v<TResult, winrt::Microsoft::Management::Deployment::UninstallResult>)
        {
            return GetUninstallResult(executionStage, terminationHR, operationError, correlationData, rebootRequired);
        }
        else if constexpr (std::is_same_v<TResult, winrt::Microsoft::Management::Deployment::DownloadResult>)
        {
            return GetDownloadResult(executionStage, terminationHR, correlationData);
        }
        else if constexpr (std::is_same_v<TResult, winrt::Microsoft::Management::Deployment::RepairResult>)
        {
            return GetRepairResult(executionStage, terminationHR, operationError, correlationData, rebootRequired);
        }
    }

#define WINGET_GET_PROGRESS_STATE(_installState_, _uninstallState_, _repairState_) \
    if constexpr (std::is_same_v<TState, winrt::Microsoft::Management::Deployment::PackageInstallProgressState>) \
    { \
        progressState = TState::_installState_; \
    } \
    else if constexpr (std::is_same_v<TState, winrt::Microsoft::Management::Deployment::PackageUninstallProgressState>) \
    { \
        progressState = TState::_uninstallState_; \
    } \
    else if constexpr (std::is_same_v<TState, winrt::Microsoft::Management::Deployment::PackageRepairProgressState>) \
    { \
        progressState = TState::_repairState_; \
    } \

    template <typename TProgress, typename TState>
    std::optional<TProgress> GetProgress(
        ReportType reportType,
        uint64_t current,
        uint64_t maximum,
        ::AppInstaller::ProgressType progressType,
        ::Workflow::ExecutionStage executionPhase)
    {
        bool reportProgress = false;
        TState progressState = TState::Queued;
        double downloadProgress = 0;
        double operationProgress = 0;
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
            if constexpr (std::is_same_v<TProgress, winrt::Microsoft::Management::Deployment::InstallProgress> ||
                std::is_same_v<TProgress, winrt::Microsoft::Management::Deployment::PackageDownloadProgress>)
            {
                progressState = TState::Downloading;
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
            }
            break;
        case ::Workflow::ExecutionStage::PreExecution:
            // Wait until installer starts to report operation.
            break;
        case ::Workflow::ExecutionStage::Execution:
            WINGET_GET_PROGRESS_STATE(Installing, Uninstalling, Repairing);
            downloadProgress = 1;
            if (reportType == ReportType::ExecutionPhaseUpdate)
            {
                // Operation is starting. Send progress so callers know the AsyncOperation can't be cancelled.
                reportProgress = true;
            }
            else if (reportType == ReportType::EndProgress)
            {
                // Operation is "finished". May not have succeeded.
                reportProgress = true;
                operationProgress = 1;
            }
            else if (progressType == ::AppInstaller::ProgressType::Percent)
            {
                if (maximum > 0 && maximum >= current)
                {
                    // Operation is progressing
                    reportProgress = true;
                    operationProgress = static_cast<double>(current) / static_cast<double>(maximum);
                }
            }
            break;
        case ::Workflow::ExecutionStage::PostExecution:
            if (reportType == ReportType::ExecutionPhaseUpdate)
            {
                // Send PostInstall progress when it switches to PostExecution phase.
                reportProgress = true;
                WINGET_GET_PROGRESS_STATE(PostInstall, PostUninstall, PostRepair);
                downloadProgress = 1;
                operationProgress = 1;
            }
            break;
        }
        if (reportProgress)
        {
            if constexpr (std::is_same_v<TProgress, winrt::Microsoft::Management::Deployment::InstallProgress>)
            {
                TProgress progress{ progressState, downloadBytesDownloaded, downloadBytesRequired, downloadProgress, operationProgress };
                return progress;
            }
            else if constexpr (std::is_same_v<TProgress, winrt::Microsoft::Management::Deployment::UninstallProgress>)
            {
                TProgress progress{ progressState, operationProgress };
                return progress;
            }
            else if constexpr (std::is_same_v<TProgress, winrt::Microsoft::Management::Deployment::PackageDownloadProgress>)
            {
                TProgress progress{ progressState, downloadBytesDownloaded, downloadBytesRequired, downloadProgress };
                return progress;
            }
            else if constexpr (std::is_same_v<TProgress, winrt::Microsoft::Management::Deployment::RepairProgress>)
            {
                TProgress progress{ progressState, operationProgress };
                return progress;
            }
        }
        else
        {
            return {};
        }
    }

    template <typename TOptions>
    Microsoft::Management::Deployment::PackageVersionInfo GetPackageVersionInfo(winrt::Microsoft::Management::Deployment::CatalogPackage package, TOptions options)
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

    void PopulateContextFromInstallOptions(
        ::AppInstaller::CLI::Execution::Context* context,
        winrt::Microsoft::Management::Deployment::InstallOptions options)
    {
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

            if (options.BypassIsStoreClientBlockedPolicyCheck())
            {
                context->SetFlags(Execution::ContextFlag::BypassIsStoreClientBlockedPolicyCheck);
            }

            if (options.Force())
            {
                context->Args.AddArg(Execution::Args::Type::Force);
            }

            // If the PackageInstallScope is anything other than ::Any then set it as a requirement.
            auto manifestScope = GetManifestScope(options.PackageInstallScope());
            if (manifestScope.first != ::AppInstaller::Manifest::ScopeEnum::Unknown)
            {
                context->Args.AddArg(Execution::Args::Type::InstallScope, ScopeToString(manifestScope.first));
                context->Add<Execution::Data::AllowUnknownScope>(manifestScope.second);
            }

            if (options.PackageInstallMode() == PackageInstallMode::Interactive)
            {
                context->Args.AddArg(Execution::Args::Type::Interactive);
            }
            else if (options.PackageInstallMode() == PackageInstallMode::Silent)
            {
                context->Args.AddArg(Execution::Args::Type::Silent);
            }

            auto installerType = GetManifestInstallerType(options.InstallerType());
            if (installerType != AppInstaller::Manifest::InstallerTypeEnum::Unknown)
            {
                context->Args.AddArg(Execution::Args::Type::InstallerType, AppInstaller::Manifest::InstallerTypeToString(installerType));
            }

            if (!options.PreferredInstallLocation().empty())
            {
                context->Args.AddArg(Execution::Args::Type::InstallLocation, ::AppInstaller::Utility::ConvertToUTF8(options.PreferredInstallLocation()));
            }

            if (!options.ReplacementInstallerArguments().empty())
            {
                context->Args.AddArg(Execution::Args::Type::Override, ::AppInstaller::Utility::ConvertToUTF8(options.ReplacementInstallerArguments()));
            }

            if (!options.AdditionalInstallerArguments().empty())
            {
                context->Args.AddArg(Execution::Args::Type::CustomSwitches, ::AppInstaller::Utility::ConvertToUTF8(options.AdditionalInstallerArguments()));
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

            // Note: AdditionalPackageCatalogArguments is not needed during install since the manifest is already known so no additional calls to the source are needed. The property is deprecated.

            if (options.AcceptPackageAgreements())
            {
                context->Args.AddArg(Execution::Args::Type::AcceptPackageAgreements);
            }

            if (options.SkipDependencies())
            {
                context->Args.AddArg(Execution::Args::Type::SkipDependencies);
            }

            if (options.AuthenticationArguments())
            {
                context->Args.AddArg(Execution::Args::Type::AuthenticationMode, ::AppInstaller::Authentication::AuthenticationModeToString(GetAuthenticationMode(options.AuthenticationArguments().AuthenticationMode())));
                context->Args.AddArg(Execution::Args::Type::AuthenticationAccount, ::AppInstaller::Utility::ConvertToUTF8(options.AuthenticationArguments().AuthenticationAccount()));
            }
        }
        else
        {
            // Note: If no install options are specified, we assume the caller is accepting the package agreements by default.
            context->Args.AddArg(Execution::Args::Type::AcceptPackageAgreements);
        }
    }

    void PopulateContextFromUninstallOptions(
        ::AppInstaller::CLI::Execution::Context* context,
        winrt::Microsoft::Management::Deployment::UninstallOptions options)
    {
        if (options)
        {
            if (!options.LogOutputPath().empty())
            {
                context->Args.AddArg(Execution::Args::Type::Log, ::AppInstaller::Utility::ConvertToUTF8(options.LogOutputPath()));
                context->Args.AddArg(Execution::Args::Type::VerboseLogs);
            }
            if (options.Force())
            {
                context->Args.AddArg(Execution::Args::Type::Force);
            }

            if (options.PackageUninstallMode() == PackageUninstallMode::Interactive)
            {
                context->Args.AddArg(Execution::Args::Type::Interactive);
            }
            else if (options.PackageUninstallMode() == PackageUninstallMode::Silent)
            {
                context->Args.AddArg(Execution::Args::Type::Silent);
            }

            auto uninstallScope = GetManifestUninstallScope(options.PackageUninstallScope());
            if (uninstallScope != ::AppInstaller::Manifest::ScopeEnum::Unknown)
            {
                context->Args.AddArg(Execution::Args::Type::InstallScope, ScopeToString(uninstallScope));
            }
        }
    }

    void PopulateContextFromDownloadOptions(
        ::AppInstaller::CLI::Execution::Context* context,
        winrt::Microsoft::Management::Deployment::DownloadOptions options)
    {
        if (options)
        {
            if (!options.DownloadDirectory().empty())
            {
                context->Args.AddArg(Execution::Args::Type::DownloadDirectory, ::AppInstaller::Utility::ConvertToUTF8(options.DownloadDirectory()));
            }
            if (!options.Locale().empty())
            {
                context->Args.AddArg(Execution::Args::Type::Locale, ::AppInstaller::Utility::ConvertToUTF8(options.Locale()));
            }
            if (options.AllowHashMismatch())
            {
                context->Args.AddArg(Execution::Args::Type::HashOverride);
            }
            if (options.SkipDependencies())
            {
                context->Args.AddArg(Execution::Args::Type::SkipDependencies);
            }
            if (options.AcceptPackageAgreements())
            {
                context->Args.AddArg(Execution::Args::Type::AcceptPackageAgreements);
            }
            auto manifestScope = GetManifestScope(options.Scope());
            if (manifestScope.first != ::AppInstaller::Manifest::ScopeEnum::Unknown)
            {
                context->Args.AddArg(Execution::Args::Type::InstallScope, ScopeToString(manifestScope.first));
            }

            auto architecture = options.Architecture();
            if (architecture != Windows::System::ProcessorArchitecture::Unknown)
            {
                auto convertedArchitecture = GetUtilityArchitecture(architecture);
                if (convertedArchitecture)
                {
                    context->Args.AddArg(Execution::Args::Type::InstallerArchitecture, ToString(convertedArchitecture.value()));
                }
            }

            auto installerType = GetManifestInstallerType(options.InstallerType());
            if (installerType != AppInstaller::Manifest::InstallerTypeEnum::Unknown)
            {
                context->Args.AddArg(Execution::Args::Type::InstallerType, AppInstaller::Manifest::InstallerTypeToString(installerType));
            }

            if (options.AuthenticationArguments())
            {
                context->Args.AddArg(Execution::Args::Type::AuthenticationMode, ::AppInstaller::Authentication::AuthenticationModeToString(GetAuthenticationMode(options.AuthenticationArguments().AuthenticationMode())));
                context->Args.AddArg(Execution::Args::Type::AuthenticationAccount, ::AppInstaller::Utility::ConvertToUTF8(options.AuthenticationArguments().AuthenticationAccount()));
            }
        }
    }

    void PopulateContextFromRepairOptions(
        ::AppInstaller::CLI::Execution::Context* context,
        winrt::Microsoft::Management::Deployment::RepairOptions options)
    {
        if (options)
        {
            if (!options.LogOutputPath().empty())
            {
                context->Args.AddArg(Execution::Args::Type::Log, ::AppInstaller::Utility::ConvertToUTF8(options.LogOutputPath()));
                context->Args.AddArg(Execution::Args::Type::VerboseLogs);
            }

            if (options.PackageRepairMode() == PackageRepairMode::Interactive)
            {
                context->Args.AddArg(Execution::Args::Type::Interactive);
            }
            else if (options.PackageRepairMode() == PackageRepairMode::Silent)
            {
                context->Args.AddArg(Execution::Args::Type::Silent);
            }

            if (options.AcceptPackageAgreements())
            {
                context->Args.AddArg(Execution::Args::Type::AcceptPackageAgreements);
            }

            if (options.AllowHashMismatch())
            {
                context->Args.AddArg(Execution::Args::Type::HashOverride);
            }

            if (options.BypassIsStoreClientBlockedPolicyCheck())
            {
                context->SetFlags(Execution::ContextFlag::BypassIsStoreClientBlockedPolicyCheck);
            }

            if (options.Force())
            {
                context->Args.AddArg(Execution::Args::Type::Force);
            }

            auto repairScope = GetManifestRepairScope(options.PackageRepairScope());
            if (repairScope != ::AppInstaller::Manifest::ScopeEnum::Unknown)
            {
                context->Args.AddArg(Execution::Args::Type::InstallScope, ScopeToString(repairScope));
            }

            if (options.AuthenticationArguments())
            {
                context->Args.AddArg(Execution::Args::Type::AuthenticationMode, ::AppInstaller::Authentication::AuthenticationModeToString(GetAuthenticationMode(options.AuthenticationArguments().AuthenticationMode())));
                context->Args.AddArg(Execution::Args::Type::AuthenticationAccount, ::AppInstaller::Utility::ConvertToUTF8(options.AuthenticationArguments().AuthenticationAccount()));
            }
        }
    }

    template <typename TOptions>
    std::unique_ptr<COMContext> CreateContextFromOperationOptions(
        TOptions options,
        std::wstring callerProcessInfoString)
    {
        std::unique_ptr<COMContext> context = std::make_unique<COMContext>();
        hstring correlationData = (options) ? options.CorrelationData() : L"";

        context->SetContextLoggers(correlationData, GetComCallerName(AppInstaller::Utility::ConvertToUTF8(callerProcessInfoString)));

        // Convert the options to arguments for the installer.
        if constexpr (std::is_same_v<TOptions, winrt::Microsoft::Management::Deployment::InstallOptions>)
        {
            PopulateContextFromInstallOptions(context.get(), options);
        }
        else if constexpr (std::is_same_v<TOptions, winrt::Microsoft::Management::Deployment::UninstallOptions>)
        {
            PopulateContextFromUninstallOptions(context.get(), options);
        }
        else if constexpr (std::is_same_v<TOptions, winrt::Microsoft::Management::Deployment::DownloadOptions>)
        {
            PopulateContextFromDownloadOptions(context.get(), options);
        }
        else if constexpr (std::is_same_v<TOptions, winrt::Microsoft::Management::Deployment::RepairOptions>)
        {
            PopulateContextFromRepairOptions(context.get(), options);
        }

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
            auto searchItem = Execution::OrchestratorQueueItemFactory::CreateItemForSearch(std::wstring{ package.Id() }, std::wstring{ catalogInfo.Id() }, std::move(context));
            queueItem = Execution::ContextOrchestrator::Instance().GetQueueItem(searchItem->GetId());
            return queueItem;
        }

        // If the caller has not specified the catalog, then check InstalledVersion. When the package comes from the Installing catalog the PackageCatalog
        // of the InstalledVersion will be set to the original catalog that the install was from, so checking the InstalledVersion first is most likely to 
        // find a result.
        Microsoft::Management::Deployment::PackageVersionInfo installedVersionInfo = package.InstalledVersion();
        if (installedVersionInfo)
        {
            auto searchItem = Execution::OrchestratorQueueItemFactory::CreateItemForSearch(std::wstring{ package.Id() }, std::wstring{ installedVersionInfo.PackageCatalog().Info().Id() }, std::move(context));
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
            auto searchItem = Execution::OrchestratorQueueItemFactory::CreateItemForSearch(std::wstring{ package.Id() }, std::wstring{ defaultInstallVersionInfo.PackageCatalog().Info().Id() }, std::move(context));
            queueItem = Execution::ContextOrchestrator::Instance().GetQueueItem(searchItem->GetId());
            if (queueItem)
            {
                return queueItem;
            }
        }

        // Finally check all catalogs in AvailableVersions.
        for (Microsoft::Management::Deployment::PackageVersionId versionId : package.AvailableVersions())
        {
            auto searchItem = Execution::OrchestratorQueueItemFactory::CreateItemForSearch(std::wstring{ package.Id() }, std::wstring{ package.GetPackageVersionInfo(versionId).PackageCatalog().Info().Id() }, std::move(context));
            queueItem = Execution::ContextOrchestrator::Instance().GetQueueItem(searchItem->GetId());
            if (queueItem)
            {
                return queueItem;
            }
        }
        return nullptr;
    }

    std::unique_ptr<Execution::OrchestratorQueueItem> CreateQueueItemForInstall(
        std::unique_ptr<::AppInstaller::CLI::Execution::COMContext> comContext,
        winrt::Microsoft::Management::Deployment::CatalogPackage package,
        winrt::Microsoft::Management::Deployment::InstallOptions options,
        bool isUpgrade)
    {
        // Add manifest and PackageVersion to context for install/upgrade.
        // If the version of the package is specified use that, otherwise use the default.
        Microsoft::Management::Deployment::PackageVersionInfo packageVersionInfo = GetPackageVersionInfo(package, options);
        AddPackageManifestToContext(packageVersionInfo, comContext.get());

        if (isUpgrade)
        {
            AppInstaller::Utility::VersionAndChannel installedVersion{ winrt::to_string(package.InstalledVersion().Version()), winrt::to_string(package.InstalledVersion().Channel()) };
            AppInstaller::Utility::VersionAndChannel upgradeVersion{ winrt::to_string(packageVersionInfo.Version()), winrt::to_string(packageVersionInfo.Channel()) };

            // Perform upgrade version check
            if (upgradeVersion.GetVersion().IsUnknown())
            {
                if (!(options.AllowUpgradeToUnknownVersion() &&
                    AppInstaller::Utility::ICUCaseInsensitiveEquals(installedVersion.GetChannel().ToString(), upgradeVersion.GetChannel().ToString())))
                {
                    THROW_HR(APPINSTALLER_CLI_ERROR_UPGRADE_VERSION_UNKNOWN);
                }
            }
            else if (!installedVersion.IsUpdatedBy(upgradeVersion))
            {
                THROW_HR(APPINSTALLER_CLI_ERROR_UPGRADE_VERSION_NOT_NEWER);
            }

            // Set upgrade flag
            comContext->SetFlags(AppInstaller::CLI::Execution::ContextFlag::InstallerExecutionUseUpdate);
            // Add installed version
            AddInstalledVersionToContext(package.InstalledVersion(), comContext.get());
        }

        return Execution::OrchestratorQueueItemFactory::CreateItemForInstall(std::wstring{ package.Id() }, std::wstring{ packageVersionInfo.PackageCatalog().Info().Id() }, std::move(comContext), isUpgrade);
    }

    std::unique_ptr<Execution::OrchestratorQueueItem> CreateQueueItemForUninstall(
        std::unique_ptr<::AppInstaller::CLI::Execution::COMContext> comContext,
        winrt::Microsoft::Management::Deployment::CatalogPackage package)
    {
        // Add installed version
        AddInstalledVersionToContext(package.InstalledVersion(), comContext.get());

        // Add Package which is used by RecordUninstall later for removing from tracking catalog of correlated available sources as best effort
        winrt::Microsoft::Management::Deployment::implementation::CatalogPackage* catalogPackageImpl = get_self<winrt::Microsoft::Management::Deployment::implementation::CatalogPackage>(package);
        std::shared_ptr<::AppInstaller::Repository::ICompositePackage> internalPackage = catalogPackageImpl->GetRepositoryPackage();
        comContext->Add<AppInstaller::CLI::Execution::Data::Package>(internalPackage);

        return Execution::OrchestratorQueueItemFactory::CreateItemForUninstall(std::wstring{ package.Id() }, std::wstring{ package.InstalledVersion().PackageCatalog().Info().Id() }, std::move(comContext));
    }

    std::unique_ptr<Execution::OrchestratorQueueItem> CreateQueueItemForDownload(
        std::unique_ptr<::AppInstaller::CLI::Execution::COMContext> comContext,
        winrt::Microsoft::Management::Deployment::CatalogPackage package,
        winrt::Microsoft::Management::Deployment::DownloadOptions options)
    {
        // Add manifest and PackageVersion to context for download.
        // If the version of the package is specified use that, otherwise use the default.
        Microsoft::Management::Deployment::PackageVersionInfo packageVersionInfo = GetPackageVersionInfo(package, options);
        AddPackageManifestToContext(packageVersionInfo, comContext.get());

        comContext->SetFlags(AppInstaller::CLI::Execution::ContextFlag::InstallerDownloadOnly);

        return Execution::OrchestratorQueueItemFactory::CreateItemForDownload(std::wstring{ package.Id() }, std::wstring{ packageVersionInfo.PackageCatalog().Info().Id() }, std::move(comContext));
    }

    std::unique_ptr<Execution::OrchestratorQueueItem> CreateQueueItemForRepair(
        std::unique_ptr<::AppInstaller::CLI::Execution::COMContext> comContext,
        winrt::Microsoft::Management::Deployment::CatalogPackage package,
        winrt::Microsoft::Management::Deployment::RepairOptions options)
    {
        // Add installed version
        AddInstalledVersionToContext(package.InstalledVersion(), comContext.get());

        // Add Package which is used to co-relate installed package with available package for repair
        winrt::Microsoft::Management::Deployment::implementation::CatalogPackage* catalogPackageImpl = get_self<winrt::Microsoft::Management::Deployment::implementation::CatalogPackage>(package);
        std::shared_ptr<::AppInstaller::Repository::ICompositePackage> internalPackage = catalogPackageImpl->GetRepositoryPackage();
        comContext->Add<AppInstaller::CLI::Execution::Data::Package>(internalPackage);

        comContext->SetFlags(AppInstaller::CLI::Execution::ContextFlag::InstallerExecutionUseRepair);

        return Execution::OrchestratorQueueItemFactory::CreateItemForRepair(std::wstring{ package.Id() }, std::wstring{ package.InstalledVersion().PackageCatalog().Info().Id() }, std::move(comContext));
    }

    template <typename TResult, typename TProgress, typename TOptions, typename TProgressState>
    winrt::Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress> GetPackageOperation(
        bool canCancelQueueItem,
        std::shared_ptr<Execution::OrchestratorQueueItem> queueItemParam,
        winrt::Microsoft::Management::Deployment::CatalogPackage package = nullptr,
        TOptions options = nullptr,
        std::wstring callerProcessInfoString = {},
        bool isUpgrade = false)
    {
        winrt::hresult terminationHR = S_OK;
        uint32_t operationError = 0;
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
                std::unique_ptr<COMContext> comContext = CreateContextFromOperationOptions<TOptions>(options, callerProcessInfoString);

                if constexpr (std::is_same_v<TOptions, winrt::Microsoft::Management::Deployment::InstallOptions>)
                {
                    queueItem = CreateQueueItemForInstall(std::move(comContext), package, options, isUpgrade);
                }
                else if constexpr (std::is_same_v<TOptions, winrt::Microsoft::Management::Deployment::UninstallOptions>)
                {
                    queueItem = CreateQueueItemForUninstall(std::move(comContext), package);
                }
                else if constexpr (std::is_same_v<TOptions, winrt::Microsoft::Management::Deployment::DownloadOptions>)
                {
                    queueItem = CreateQueueItemForDownload(std::move(comContext), package, options);
                }
                else if constexpr (std::is_same_v<TOptions, winrt::Microsoft::Management::Deployment::RepairOptions>)
                {
                    queueItem = CreateQueueItemForRepair(std::move(comContext), package, options);
                }

                Execution::ContextOrchestrator::Instance().EnqueueAndRunItem(queueItem);

                if constexpr (std::is_same_v<TProgress, winrt::Microsoft::Management::Deployment::PackageInstallProgressState>)
                {
                    TProgress queuedProgress{ TProgressState::Queued, 0, 0, 0 };
                    report_progress(queuedProgress);
                }
                else if constexpr (std::is_same_v<TProgress, winrt::Microsoft::Management::Deployment::PackageUninstallProgressState>)
                {
                    TProgress queuedProgress{ TProgressState::Queued, 0 };
                    report_progress(queuedProgress);
                }
                else if constexpr (std::is_same_v<TProgress, winrt::Microsoft::Management::Deployment::PackageDownloadProgressState>)
                {
                    TProgress queuedProgress{ TProgressState::Queued, 0 };
                    report_progress(queuedProgress);
                }
                else if constexpr (std::is_same_v<TProgress, winrt::Microsoft::Management::Deployment::PackageRepairProgressState>)
                {
                    TProgress queuedProgress{ TProgressState::Queued, 0 };
                    report_progress(queuedProgress);
                }
            }
            {
                // correlation data is not passed in when retrieving an existing queue item, so get it from the existing context.
                correlationData = hstring(queueItem->GetContext().GetCorrelationJson());
            }

            wil::unique_event progressEvent{ wil::EventOptions::None };

            std::atomic<TProgress> operationProgress;
            queueItem->GetContext().AddProgressCallbackFunction([&operationProgress, &progressEvent](
                ReportType reportType,
                uint64_t current,
                uint64_t maximum,
                ::AppInstaller::ProgressType progressType,
                ::Workflow::ExecutionStage executionPhase)
                {
                    std::optional<TProgress> operationProgressOptional = GetProgress<TProgress, TProgressState>(reportType, current, maximum, progressType, executionPhase);
                    if (operationProgressOptional.has_value())
                    {
                        operationProgress = operationProgressOptional.value();
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
                    report_progress(operationProgress);
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
                if (queueItem->GetContext().Contains(Data::OperationReturnCode))
                {
                    operationError = static_cast<uint32_t>(queueItem->GetContext().Get<Data::OperationReturnCode>());
                }
            }
        }
        WINGET_CATCH_STORE(terminationHR, APPINSTALLER_CLI_ERROR_COMMAND_FAILED);

        // TODO - RebootRequired not yet populated, msi arguments not returned from Execute.
        co_return GetOperationResult<TResult>(executionStage, terminationHR, operationError, correlationData, false);
    }

    template <typename TResult, typename TProgress>
    winrt::Windows::Foundation::IAsyncOperationWithProgress<TResult, TProgress> GetEmptyAsynchronousResultForOperation(
        HRESULT hr,
        hstring correlationData)
    {
        // If a function uses co_await or co_return (i.e. if it is a co_routine), it cannot use return directly.
        // This helper helps a function that is not a coroutine itself to return errors asynchronously.
        co_return GetOperationResult<TResult>(::Workflow::ExecutionStage::Initial, hr, 0, correlationData, false);
    }

#define WINGET_RETURN_INSTALL_RESULT_HR_IF(hr, boolVal) { if(boolVal) { return GetEmptyAsynchronousResultForOperation<Deployment::InstallResult, Deployment::InstallProgress>(hr, correlationData); }}
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

        return GetPackageOperation<Deployment::InstallResult, Deployment::InstallProgress, Deployment::InstallOptions, Deployment::PackageInstallProgressState>(
            true /*canCancelQueueItem*/, nullptr /*queueItem*/, package, options, std::move(callerProcessInfoString));
    }

    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> PackageManager::UpgradePackageAsync(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::InstallOptions options)
    {
        hstring correlationData = (options) ? options.CorrelationData() : L"";

        // options and catalog can both be null, package must be set.
        WINGET_RETURN_INSTALL_RESULT_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS, !package);
        // the package should have an installed version to be upgraded.
        WINGET_RETURN_INSTALL_RESULT_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS, !package.InstalledVersion());

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

        return GetPackageOperation<Deployment::InstallResult, Deployment::InstallProgress, Deployment::InstallOptions, Deployment::PackageInstallProgressState>(
            true /*canCancelQueueItem*/, nullptr /*queueItem*/, package, options, std::move(callerProcessInfoString), true /* isUpgrade */);
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
            if (queueItem == nullptr ||
                (queueItem->GetPackageOperationType() != PackageOperationType::Install && queueItem->GetPackageOperationType() != PackageOperationType::Upgrade))
            {
                return nullptr;
            }
        }
        WINGET_CATCH_STORE(hr, APPINSTALLER_CLI_ERROR_COMMAND_FAILED);
        WINGET_RETURN_INSTALL_RESULT_HR_IF_FAILED(hr);

        return GetPackageOperation<Deployment::InstallResult, Deployment::InstallProgress, Deployment::InstallOptions, Deployment::PackageInstallProgressState>(
            canCancelQueueItem, std::move(queueItem));
    }

#define WINGET_RETURN_UNINSTALL_RESULT_HR_IF(hr, boolVal) { if(boolVal) { return GetEmptyAsynchronousResultForOperation<Deployment::UninstallResult, Deployment::UninstallProgress>(hr, correlationData); }}
#define WINGET_RETURN_UNINSTALL_RESULT_HR_IF_FAILED(hr) { WINGET_RETURN_UNINSTALL_RESULT_HR_IF(hr, FAILED(hr)) }

    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::UninstallResult, winrt::Microsoft::Management::Deployment::UninstallProgress> PackageManager::UninstallPackageAsync(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::UninstallOptions options)
    {
        hstring correlationData = (options) ? options.CorrelationData() : L"";

        // options and catalog can both be null, package must be set.
        WINGET_RETURN_UNINSTALL_RESULT_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS, !package);
        // the package should have an installed version to be uninstalled.
        WINGET_RETURN_UNINSTALL_RESULT_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS, !package.InstalledVersion());

        HRESULT hr = S_OK;
        std::wstring callerProcessInfoString;
        try
        {
            // Check for permissions and get caller info for telemetry.
            // This must be done before any co_awaits since it requires info from the rpc caller thread.
            auto [hrGetCallerId, callerProcessId] = GetCallerProcessId();
            WINGET_RETURN_UNINSTALL_RESULT_HR_IF_FAILED(hrGetCallerId);
            WINGET_RETURN_UNINSTALL_RESULT_HR_IF_FAILED(EnsureProcessHasCapability(Capability::PackageManagement, callerProcessId));
            callerProcessInfoString = TryGetCallerProcessInfo(callerProcessId);
        }
        WINGET_CATCH_STORE(hr, APPINSTALLER_CLI_ERROR_COMMAND_FAILED);
        WINGET_RETURN_UNINSTALL_RESULT_HR_IF_FAILED(hr);

        return GetPackageOperation<Deployment::UninstallResult, Deployment::UninstallProgress, Deployment::UninstallOptions, Deployment::PackageUninstallProgressState>(
            true /*canCancelQueueItem*/, nullptr /*queueItem*/, package, options, std::move(callerProcessInfoString));
    }

    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::UninstallResult, winrt::Microsoft::Management::Deployment::UninstallProgress> PackageManager::GetUninstallProgress(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::PackageCatalogInfo catalogInfo)
    {
        hstring correlationData;
        WINGET_RETURN_UNINSTALL_RESULT_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS, !package);

        HRESULT hr = S_OK;
        std::shared_ptr<Execution::OrchestratorQueueItem> queueItem = nullptr;
        bool canCancelQueueItem = false;
        try
        {
            // Check for permissions
            // This must be done before any co_awaits since it requires info from the rpc caller thread.
            auto [hrGetCallerId, callerProcessId] = GetCallerProcessId();
            WINGET_RETURN_UNINSTALL_RESULT_HR_IF_FAILED(hrGetCallerId);
            canCancelQueueItem = SUCCEEDED(EnsureProcessHasCapability(Capability::PackageManagement, callerProcessId));
            if (!canCancelQueueItem)
            {
                WINGET_RETURN_UNINSTALL_RESULT_HR_IF_FAILED(EnsureProcessHasCapability(Capability::PackageQuery, callerProcessId));
            }

            // Get the queueItem synchronously.
            queueItem = GetExistingQueueItemForPackage(package, catalogInfo);
            if (queueItem == nullptr ||
                queueItem->GetPackageOperationType() != PackageOperationType::Uninstall)
            {
                return nullptr;
            }
        }
        WINGET_CATCH_STORE(hr, APPINSTALLER_CLI_ERROR_COMMAND_FAILED);
        WINGET_RETURN_UNINSTALL_RESULT_HR_IF_FAILED(hr);

        return GetPackageOperation<Deployment::UninstallResult, Deployment::UninstallProgress, Deployment::UninstallOptions, Deployment::PackageUninstallProgressState>(
            canCancelQueueItem, std::move(queueItem));
    }

#define WINGET_RETURN_DOWNLOAD_RESULT_HR_IF(hr, boolVal) { if(boolVal) { return GetEmptyAsynchronousResultForOperation<Deployment::DownloadResult, Deployment::PackageDownloadProgress>(hr, correlationData); }}
#define WINGET_RETURN_DOWNLOAD_RESULT_HR_IF_FAILED(hr) { WINGET_RETURN_DOWNLOAD_RESULT_HR_IF(hr, FAILED(hr)) }

    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::DownloadResult, winrt::Microsoft::Management::Deployment::PackageDownloadProgress> PackageManager::DownloadPackageAsync(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::DownloadOptions options)
    {
        hstring correlationData = (options) ? options.CorrelationData() : L"";

        // options and catalog can both be null, package must be set.
        WINGET_RETURN_DOWNLOAD_RESULT_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS, !package);

        HRESULT hr = S_OK;
        std::wstring callerProcessInfoString;
        try
        {
            // Check for permissions and get caller info for telemetry.
            // This must be done before any co_awaits since it requires info from the rpc caller thread.
            auto [hrGetCallerId, callerProcessId] = GetCallerProcessId();
            WINGET_RETURN_DOWNLOAD_RESULT_HR_IF_FAILED(hrGetCallerId);
            WINGET_RETURN_DOWNLOAD_RESULT_HR_IF_FAILED(EnsureComCallerHasCapability(Capability::PackageQuery));
            callerProcessInfoString = TryGetCallerProcessInfo(callerProcessId);
        }
        WINGET_CATCH_STORE(hr, APPINSTALLER_CLI_ERROR_COMMAND_FAILED);
        WINGET_RETURN_DOWNLOAD_RESULT_HR_IF_FAILED(hr);

        return GetPackageOperation<Deployment::DownloadResult, Deployment::PackageDownloadProgress, Deployment::DownloadOptions, Deployment::PackageDownloadProgressState>(
            true /*canCancelQueueItem*/, nullptr /*queueItem*/, package, options, std::move(callerProcessInfoString));
    }

    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::DownloadResult, winrt::Microsoft::Management::Deployment::PackageDownloadProgress> PackageManager::GetDownloadProgress(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::PackageCatalogInfo catalogInfo)
    {
        hstring correlationData;
        WINGET_RETURN_DOWNLOAD_RESULT_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS, !package);

        HRESULT hr = S_OK;
        std::shared_ptr<Execution::OrchestratorQueueItem> queueItem = nullptr;
        try
        {
            WINGET_RETURN_DOWNLOAD_RESULT_HR_IF_FAILED(EnsureComCallerHasCapability(Capability::PackageQuery));

            // Get the queueItem synchronously.
            queueItem = GetExistingQueueItemForPackage(package, catalogInfo);
            if (queueItem == nullptr ||
                queueItem->GetPackageOperationType() != PackageOperationType::Download)
            {
                return nullptr;
            }
        }
        WINGET_CATCH_STORE(hr, APPINSTALLER_CLI_ERROR_COMMAND_FAILED);
        WINGET_RETURN_DOWNLOAD_RESULT_HR_IF_FAILED(hr);

        return GetPackageOperation<Deployment::DownloadResult, Deployment::PackageDownloadProgress, Deployment::DownloadOptions, Deployment::PackageDownloadProgressState>(true, std::move(queueItem));
    }

#define WINGET_RETURN_REPAIR_RESULT_HR_IF(hr, boolVal) { if(boolVal) { return GetEmptyAsynchronousResultForOperation<Deployment::RepairResult, Deployment::RepairProgress>(hr, correlationData); }}
#define WINGET_RETURN_REPAIR_RESULT_HR_IF_FAILED(hr) { WINGET_RETURN_REPAIR_RESULT_HR_IF(hr, FAILED(hr)) }

    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::RepairResult, winrt::Microsoft::Management::Deployment::RepairProgress> PackageManager::RepairPackageAsync(winrt::Microsoft::Management::Deployment::CatalogPackage package, winrt::Microsoft::Management::Deployment::RepairOptions options)
    {
        hstring correlationData = (options) ? options.CorrelationData() : L"";

        // options and catalog can both be null, package must be set.
        WINGET_RETURN_REPAIR_RESULT_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS, !package);
        // the package should have an installed version to be repaired.
        WINGET_RETURN_REPAIR_RESULT_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS, !package.InstalledVersion());

        HRESULT hr = S_OK;
        std::wstring callerProcessInfoString;
        try
        {
            // Check for permissions and get caller info for telemetry.
            // This must be done before any co_awaits since it requires info from the rpc caller thread.
            auto [hrGetCallerId, callerProcessId] = GetCallerProcessId();
            WINGET_RETURN_REPAIR_RESULT_HR_IF_FAILED(hrGetCallerId);
            WINGET_RETURN_REPAIR_RESULT_HR_IF_FAILED(EnsureProcessHasCapability(Capability::PackageManagement, callerProcessId));
            callerProcessInfoString = TryGetCallerProcessInfo(callerProcessId);
        }
        WINGET_CATCH_STORE(hr, APPINSTALLER_CLI_ERROR_COMMAND_FAILED);
        WINGET_RETURN_REPAIR_RESULT_HR_IF_FAILED(hr);

        return GetPackageOperation<Deployment::RepairResult, Deployment::RepairProgress, Deployment::RepairOptions, Deployment::PackageRepairProgressState>(
            true /*canCancelQueueItem*/, nullptr /*queueItem*/, package, options, std::move(callerProcessInfoString));
    }

    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::AddPackageCatalogResult, double> PackageManager::AddPackageCatalogAsync(winrt::Microsoft::Management::Deployment::AddPackageCatalogOptions options)
    {
        LogStartupIfApplicable();

        // options must be set.
        THROW_HR_IF_NULL(E_POINTER, options);
        THROW_HR_IF(E_INVALIDARG, options.Name().empty());
        THROW_HR_IF(E_INVALIDARG, options.SourceUri().empty());

        HRESULT terminationHR = S_OK;
        try {

            // Check if running as admin/system.
            // [NOTE:] For OutOfProc calls, the Windows Package Manager Service executes in the context initiated by the caller process,
            // so the same admin/system validation check is applicable for both InProc and OutOfProc calls.
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_COMMAND_REQUIRES_ADMIN, !AppInstaller::Runtime::IsRunningAsAdminOrSystem());

            ::AppInstaller::Repository::Source sourceToAdd = CreateSourceFromOptions(options);

            auto strong_this = get_strong();
            auto report_progress{ co_await winrt::get_progress_token() };
            co_await winrt::resume_background();

            std::string type = winrt::to_string(options.Type());
            auto packageCatalogProgressSink = winrt::Microsoft::Management::Deployment::ProgressSinkFactory::CreatePackageCatalogProgressSink(type, report_progress );

            packageCatalogProgressSink->BeginProgress();
            ::AppInstaller::ProgressCallback progress(packageCatalogProgressSink.get());
            sourceToAdd.Add(progress);
            packageCatalogProgressSink->EndProgress(false);
        }
        catch (...)
        {
            terminationHR = AppInstaller::CLI::Workflow::HandleException(nullptr, std::current_exception());
        }

        co_return GetAddPackageCatalogResult(terminationHR);
    }

    winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::RemovePackageCatalogResult, double> PackageManager::RemovePackageCatalogAsync(winrt::Microsoft::Management::Deployment::RemovePackageCatalogOptions options)
    {
        LogStartupIfApplicable();

        // options must be set.
        THROW_HR_IF_NULL(E_POINTER, options);
        THROW_HR_IF(E_INVALIDARG, options.Name().empty());

        HRESULT terminationHR = S_OK;
        try {

            // Check if running as admin/system.
            // [NOTE:] For OutOfProc calls, the Windows Package Manager Service executes in the context initiated by the caller process,
            // so the same admin/system validation check is applicable for both InProc and OutOfProc calls.
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_COMMAND_REQUIRES_ADMIN, !AppInstaller::Runtime::IsRunningAsAdminOrSystem());

            auto matchingSource = GetMatchingSource(winrt::to_string(options.Name()));
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_NAME_DOES_NOT_EXIST, !matchingSource.has_value());

            auto strong_this = get_strong();
            auto report_progress{ co_await winrt::get_progress_token() };
            co_await winrt::resume_background();

            auto packageCatalogProgressSink = winrt::Microsoft::Management::Deployment::ProgressSinkFactory::CreatePackageCatalogProgressSink(matchingSource.value().Type, report_progress, true);

            packageCatalogProgressSink->BeginProgress();
            ::AppInstaller::Repository::Source sourceToRemove = ::AppInstaller::Repository::Source{ matchingSource.value().Name };
            ::AppInstaller::ProgressCallback progress(packageCatalogProgressSink.get());

            // If the PreserveData option is set, this is equivalent to the WinGet CLI Reset command on a single source; otherwise, it removes the source.
            if (options.PreserveData())
            {
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_NAME_DOES_NOT_EXIST, !sourceToRemove.DropSource(matchingSource.value().Name));
            }
            else
            {
                sourceToRemove.Remove(progress);
            }
            packageCatalogProgressSink->EndProgress(false);
        }
        catch (...)
        {
            terminationHR = AppInstaller::CLI::Workflow::HandleException(nullptr, std::current_exception());
        }

        co_return GetRemovePackageCatalogResult(terminationHR);
    }

    CoCreatableMicrosoftManagementDeploymentClass(PackageManager);
}
