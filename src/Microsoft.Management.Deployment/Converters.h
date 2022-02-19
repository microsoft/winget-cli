// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageMatchFilter.g.h"
#include <AppInstallerArchitecture.h>
#include <winget/RepositorySource.h>
#include <Workflows/WorkflowBase.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Microsoft::Management::Deployment::PackageMatchField GetDeploymentMatchField(::AppInstaller::Repository::PackageMatchField field);
    ::AppInstaller::Repository::PackageMatchField GetRepositoryMatchField(winrt::Microsoft::Management::Deployment::PackageMatchField field);
    winrt::Microsoft::Management::Deployment::PackageFieldMatchOption GetDeploymentMatchOption(::AppInstaller::Repository::MatchType type);
    ::AppInstaller::Repository::MatchType GetRepositoryMatchType(winrt::Microsoft::Management::Deployment::PackageFieldMatchOption option);
    ::AppInstaller::Repository::CompositeSearchBehavior GetRepositoryCompositeSearchBehavior(winrt::Microsoft::Management::Deployment::CompositeSearchBehavior searchBehavior);
    ::AppInstaller::Repository::PackageVersionMetadata GetRepositoryPackageVersionMetadata(winrt::Microsoft::Management::Deployment::PackageVersionMetadataField packageVersionMetadataField);
    winrt::Microsoft::Management::Deployment::FindPackagesResultStatus FindPackagesResultStatus(winrt::hresult hresult);
    std::optional<::AppInstaller::Utility::Architecture> GetUtilityArchitecture(winrt::Windows::System::ProcessorArchitecture architecture);
    std::optional<winrt::Windows::System::ProcessorArchitecture> GetWindowsSystemProcessorArchitecture(::AppInstaller::Utility::Architecture architecture);

#define WINGET_GET_OPERATION_RESULT_STATUS(_installResultStatus_, _uninstallResultStatus_) \
    if constexpr (std::is_same_v<TStatus, winrt::Microsoft::Management::Deployment::InstallResultStatus>) \
    { \
        resultStatus = TStatus::_installResultStatus_; \
    } \
    else if constexpr (std::is_same_v<TStatus, winrt::Microsoft::Management::Deployment::UninstallResultStatus>) \
    { \
        resultStatus = TStatus::_uninstallResultStatus_; \
    } \

    template <typename TStatus>
    TStatus GetOperationResultStatus(::AppInstaller::CLI::Workflow::ExecutionStage executionStage, winrt::hresult hresult)
    {
        TStatus resultStatus = TStatus::Ok;

        // Map some known hresults to specific statuses, otherwise use the execution stage to determine the status.
        switch (hresult)
        {
        case S_OK:
            resultStatus = TStatus::Ok;
            break;
        case APPINSTALLER_CLI_ERROR_MSSTORE_BLOCKED_BY_POLICY:
        case APPINSTALLER_CLI_ERROR_MSSTORE_APP_BLOCKED_BY_POLICY:
        case APPINSTALLER_CLI_ERROR_EXPERIMENTAL_FEATURE_DISABLED:
        case APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY:
            resultStatus = TStatus::BlockedByPolicy;
            break;
        case APPINSTALLER_CLI_ERROR_INVALID_MANIFEST:
            resultStatus = TStatus::ManifestError;
            break;
        case E_INVALIDARG:
        case APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS:
            resultStatus = TStatus::InvalidOptions;
            break;
        case APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER:
            WINGET_GET_OPERATION_RESULT_STATUS(NoApplicableInstallers, InternalError);
            break;
        case APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE:
        case APPINSTALLER_CLI_ERROR_UPGRADE_VERSION_UNKNOWN:
        case APPINSTALLER_CLI_ERROR_UPGRADE_VERSION_NOT_NEWER:
            WINGET_GET_OPERATION_RESULT_STATUS(NoApplicableUpgrade, InternalError);
            break;
        case APPINSTALLER_CLI_ERROR_NO_UNINSTALL_INFO_FOUND:
        case APPINSTALLER_CLI_ERROR_EXEC_UNINSTALL_COMMAND_FAILED:
            WINGET_GET_OPERATION_RESULT_STATUS(InstallError, UninstallError);
            break;
        case APPINSTALLER_CLI_ERROR_CANNOT_WRITE_TO_UPLEVEL_INDEX:
        case APPINSTALLER_CLI_ERROR_INDEX_INTEGRITY_COMPROMISED:
        case APPINSTALLER_CLI_ERROR_YAML_INIT_FAILED:
        case APPINSTALLER_CLI_ERROR_YAML_INVALID_MAPPING_KEY:
        case APPINSTALLER_CLI_ERROR_YAML_DUPLICATE_MAPPING_KEY:
        case APPINSTALLER_CLI_ERROR_YAML_INVALID_OPERATION:
        case APPINSTALLER_CLI_ERROR_YAML_DOC_BUILD_FAILED:
        case APPINSTALLER_CLI_ERROR_YAML_INVALID_EMITTER_STATE:
        case APPINSTALLER_CLI_ERROR_YAML_INVALID_DATA:
        case APPINSTALLER_CLI_ERROR_LIBYAML_ERROR:
        case APPINSTALLER_CLI_ERROR_INTERNAL_ERROR:
            resultStatus = TStatus::InternalError;
            break;
        default:
            switch (executionStage)
            {
            case ::AppInstaller::CLI::Workflow::ExecutionStage::Initial:
                resultStatus = TStatus::InternalError;
                break;
            case ::AppInstaller::CLI::Workflow::ExecutionStage::ParseArgs:
                resultStatus = TStatus::InvalidOptions;
                break;
            case ::AppInstaller::CLI::Workflow::ExecutionStage::Discovery:
                resultStatus = TStatus::CatalogError;
                break;
            case ::AppInstaller::CLI::Workflow::ExecutionStage::Download:
                WINGET_GET_OPERATION_RESULT_STATUS(DownloadError, InternalError);
                break;
            case ::AppInstaller::CLI::Workflow::ExecutionStage::PreExecution:
                resultStatus = TStatus::InternalError;
                break;
            case ::AppInstaller::CLI::Workflow::ExecutionStage::Execution:
                WINGET_GET_OPERATION_RESULT_STATUS(InstallError, UninstallError);
                break;
            case ::AppInstaller::CLI::Workflow::ExecutionStage::PostExecution:
                resultStatus = TStatus::InternalError;
                break;
            default:
                resultStatus = TStatus::InternalError;
                break;
            }
        }

        return resultStatus;
    }
}
