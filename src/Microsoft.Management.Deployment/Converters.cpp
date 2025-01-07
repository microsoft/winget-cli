// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <AppInstallerErrors.h>
#include <winget/RepositorySource.h>
#include "Microsoft/PredefinedInstalledSourceFactory.h"
#include "Workflows/WorkflowBase.h"
#include "Converters.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    Microsoft::Management::Deployment::PackageMatchField GetDeploymentMatchField(::AppInstaller::Repository::PackageMatchField field)
    {
        Microsoft::Management::Deployment::PackageMatchField matchField = Microsoft::Management::Deployment::PackageMatchField::Id;
        switch (field)
        {
        case ::AppInstaller::Repository::PackageMatchField::Command:
            matchField = Microsoft::Management::Deployment::PackageMatchField::Command;
            break;
        case ::AppInstaller::Repository::PackageMatchField::Id:
            matchField = Microsoft::Management::Deployment::PackageMatchField::Id;
            break;
        case ::AppInstaller::Repository::PackageMatchField::Moniker:
            matchField = Microsoft::Management::Deployment::PackageMatchField::Moniker;
            break;
        case ::AppInstaller::Repository::PackageMatchField::Name:
            matchField = Microsoft::Management::Deployment::PackageMatchField::Name;
            break;
        case ::AppInstaller::Repository::PackageMatchField::Tag:
            matchField = Microsoft::Management::Deployment::PackageMatchField::Tag;
            break;
        case ::AppInstaller::Repository::PackageMatchField::ProductCode:
            matchField = Microsoft::Management::Deployment::PackageMatchField::ProductCode;
            break;
        case ::AppInstaller::Repository::PackageMatchField::PackageFamilyName:
            matchField = Microsoft::Management::Deployment::PackageMatchField::PackageFamilyName;
            break;
        default:
            matchField = Microsoft::Management::Deployment::PackageMatchField::Id;
            break;
        }
        return matchField;
    }

    ::AppInstaller::Repository::PackageMatchField GetRepositoryMatchField(Microsoft::Management::Deployment::PackageMatchField field)
    {
        ::AppInstaller::Repository::PackageMatchField matchField = ::AppInstaller::Repository::PackageMatchField::Id;
        switch (field)
        {
        case Microsoft::Management::Deployment::PackageMatchField::Command:
            matchField = ::AppInstaller::Repository::PackageMatchField::Command;
            break;
        case Microsoft::Management::Deployment::PackageMatchField::Id:
            matchField = ::AppInstaller::Repository::PackageMatchField::Id;
            break;
        case Microsoft::Management::Deployment::PackageMatchField::Moniker:
            matchField = ::AppInstaller::Repository::PackageMatchField::Moniker;
            break;
        case Microsoft::Management::Deployment::PackageMatchField::Name:
            matchField = ::AppInstaller::Repository::PackageMatchField::Name;
            break;
        case Microsoft::Management::Deployment::PackageMatchField::Tag:
            matchField = ::AppInstaller::Repository::PackageMatchField::Tag;
            break;
        case Microsoft::Management::Deployment::PackageMatchField::ProductCode:
            matchField = ::AppInstaller::Repository::PackageMatchField::ProductCode;
            break;
        case Microsoft::Management::Deployment::PackageMatchField::PackageFamilyName:
            matchField = ::AppInstaller::Repository::PackageMatchField::PackageFamilyName;
            break;
        default:
            matchField = ::AppInstaller::Repository::PackageMatchField::Id;
            break;
        }
        return matchField;
    }

    Microsoft::Management::Deployment::PackageFieldMatchOption GetDeploymentMatchOption(::AppInstaller::Repository::MatchType type)
    {
        Microsoft::Management::Deployment::PackageFieldMatchOption matchOption = Microsoft::Management::Deployment::PackageFieldMatchOption::Equals;
        switch (type)
        {
        case ::AppInstaller::Repository::MatchType::CaseInsensitive:
            matchOption = Microsoft::Management::Deployment::PackageFieldMatchOption::EqualsCaseInsensitive;
            break;
        case ::AppInstaller::Repository::MatchType::Exact:
            matchOption = Microsoft::Management::Deployment::PackageFieldMatchOption::Equals;
            break;
        case ::AppInstaller::Repository::MatchType::StartsWith:
            matchOption = Microsoft::Management::Deployment::PackageFieldMatchOption::StartsWithCaseInsensitive;
            break;
        case ::AppInstaller::Repository::MatchType::Substring:
            matchOption = Microsoft::Management::Deployment::PackageFieldMatchOption::ContainsCaseInsensitive;
            break;
        default:
            matchOption = Microsoft::Management::Deployment::PackageFieldMatchOption::Equals;
            break;
        }
        return matchOption;
    }

    ::AppInstaller::Repository::MatchType GetRepositoryMatchType(Microsoft::Management::Deployment::PackageFieldMatchOption option)
    {
        ::AppInstaller::Repository::MatchType packageFieldMatchOption = ::AppInstaller::Repository::MatchType::Exact;
        switch (option)
        {
        case Microsoft::Management::Deployment::PackageFieldMatchOption::EqualsCaseInsensitive:
            packageFieldMatchOption = ::AppInstaller::Repository::MatchType::CaseInsensitive;
            break;
        case Microsoft::Management::Deployment::PackageFieldMatchOption::Equals:
            packageFieldMatchOption = ::AppInstaller::Repository::MatchType::Exact;
            break;
        case Microsoft::Management::Deployment::PackageFieldMatchOption::StartsWithCaseInsensitive:
            packageFieldMatchOption = ::AppInstaller::Repository::MatchType::StartsWith;
            break;
        case Microsoft::Management::Deployment::PackageFieldMatchOption::ContainsCaseInsensitive:
            packageFieldMatchOption = ::AppInstaller::Repository::MatchType::Substring;
            break;
        default:
            packageFieldMatchOption = ::AppInstaller::Repository::MatchType::Exact;
            break;
        }
        return packageFieldMatchOption;
    }

    ::AppInstaller::Repository::CompositeSearchBehavior GetRepositoryCompositeSearchBehavior(Microsoft::Management::Deployment::CompositeSearchBehavior searchBehavior)
    {
        ::AppInstaller::Repository::CompositeSearchBehavior repositorySearchBehavior = ::AppInstaller::Repository::CompositeSearchBehavior::AllPackages;
        switch (searchBehavior)
        {
        case Microsoft::Management::Deployment::CompositeSearchBehavior::LocalCatalogs:
            repositorySearchBehavior = ::AppInstaller::Repository::CompositeSearchBehavior::Installed;
            break;
        case Microsoft::Management::Deployment::CompositeSearchBehavior::RemotePackagesFromRemoteCatalogs:
            repositorySearchBehavior = ::AppInstaller::Repository::CompositeSearchBehavior::AvailablePackages;
            break;
        case Microsoft::Management::Deployment::CompositeSearchBehavior::RemotePackagesFromAllCatalogs:
            repositorySearchBehavior = ::AppInstaller::Repository::CompositeSearchBehavior::AvailablePackages;
            break;
        case Microsoft::Management::Deployment::CompositeSearchBehavior::AllCatalogs:
        default:
            repositorySearchBehavior = ::AppInstaller::Repository::CompositeSearchBehavior::AllPackages;
            break;
        }
        return repositorySearchBehavior;
    }

    ::AppInstaller::Repository::PackageVersionMetadata GetRepositoryPackageVersionMetadata(Microsoft::Management::Deployment::PackageVersionMetadataField packageVersionMetadataField)
    {
        ::AppInstaller::Repository::PackageVersionMetadata metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::InstalledLocation;
        switch (packageVersionMetadataField)
        {
        case Microsoft::Management::Deployment::PackageVersionMetadataField::InstalledLocation:
            metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::InstalledLocation;
            break;
        case Microsoft::Management::Deployment::PackageVersionMetadataField::InstalledScope:
            metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::InstalledScope;
            break;
        case Microsoft::Management::Deployment::PackageVersionMetadataField::InstallerType:
            metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::InstalledType;
            break;
        case Microsoft::Management::Deployment::PackageVersionMetadataField::PublisherDisplayName:
            metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::Publisher;
            break;
        case Microsoft::Management::Deployment::PackageVersionMetadataField::SilentUninstallCommand:
            metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::SilentUninstallCommand;
            break;
        case Microsoft::Management::Deployment::PackageVersionMetadataField::StandardUninstallCommand:
            metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::StandardUninstallCommand;
            break;
        }
        return metadataKey;
    }

    winrt::Microsoft::Management::Deployment::FindPackagesResultStatus FindPackagesResultStatus(winrt::hresult hresult)
    {
        winrt::Microsoft::Management::Deployment::FindPackagesResultStatus resultStatus = winrt::Microsoft::Management::Deployment::FindPackagesResultStatus::Ok;
        switch (hresult)
        {
        case(S_OK):
            resultStatus = winrt::Microsoft::Management::Deployment::FindPackagesResultStatus::Ok;
            break;
        case APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY:
            resultStatus = winrt::Microsoft::Management::Deployment::FindPackagesResultStatus::BlockedByPolicy;
            break;
        case APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE:
        case APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA:
        case APPINSTALLER_CLI_ERROR_RESTAPI_ENDPOINT_NOT_FOUND:
        case APPINSTALLER_CLI_ERROR_RESTAPI_INTERNAL_ERROR:
        case APPINSTALLER_CLI_ERROR_RESTAPI_UNSUPPORTED_MIME_TYPE:
        case APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_VERSION:
        case APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE:
            resultStatus = winrt::Microsoft::Management::Deployment::FindPackagesResultStatus::CatalogError;
            break;
        case E_INVALIDARG:
        case APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS:
            resultStatus = winrt::Microsoft::Management::Deployment::FindPackagesResultStatus::InvalidOptions;
            break;
        case APPINSTALLER_CLI_ERROR_INVALID_AUTHENTICATION_INFO:
        case APPINSTALLER_CLI_ERROR_AUTHENTICATION_TYPE_NOT_SUPPORTED:
        case APPINSTALLER_CLI_ERROR_AUTHENTICATION_FAILED:
        case APPINSTALLER_CLI_ERROR_AUTHENTICATION_INTERACTIVE_REQUIRED:
        case APPINSTALLER_CLI_ERROR_AUTHENTICATION_CANCELLED_BY_USER:
        case APPINSTALLER_CLI_ERROR_AUTHENTICATION_INCORRECT_ACCOUNT:
            resultStatus = winrt::Microsoft::Management::Deployment::FindPackagesResultStatus::AuthenticationError;
            break;
        case HTTP_E_STATUS_DENIED:
        case HTTP_E_STATUS_FORBIDDEN:
            resultStatus = winrt::Microsoft::Management::Deployment::FindPackagesResultStatus::AccessDenied;
            break;
        case APPINSTALLER_CLI_ERROR_COMMAND_FAILED:
        case APPINSTALLER_CLI_ERROR_CANNOT_WRITE_TO_UPLEVEL_INDEX:
        case APPINSTALLER_CLI_ERROR_INDEX_INTEGRITY_COMPROMISED:
        default:
            resultStatus = winrt::Microsoft::Management::Deployment::FindPackagesResultStatus::InternalError;
            break;
        }
        return resultStatus;
    }

    std::optional<::AppInstaller::Utility::Architecture> GetUtilityArchitecture(winrt::Windows::System::ProcessorArchitecture architecture)
    {
        return ::AppInstaller::Utility::ConvertToArchitectureEnum(architecture);
    }

    std::optional<winrt::Windows::System::ProcessorArchitecture> GetWindowsSystemProcessorArchitecture(::AppInstaller::Utility::Architecture architecture)
    {
        switch (architecture)
        {
        case ::AppInstaller::Utility::Architecture::X86:
            return winrt::Windows::System::ProcessorArchitecture::X86;
        case ::AppInstaller::Utility::Architecture::Arm:
            return winrt::Windows::System::ProcessorArchitecture::Arm;
        case ::AppInstaller::Utility::Architecture::X64:
            return winrt::Windows::System::ProcessorArchitecture::X64;
        case ::AppInstaller::Utility::Architecture::Neutral:
            return winrt::Windows::System::ProcessorArchitecture::Neutral;
        case ::AppInstaller::Utility::Architecture::Arm64:
            return winrt::Windows::System::ProcessorArchitecture::Arm64;
        }

        return {};
    }

    std::pair<::AppInstaller::Manifest::ScopeEnum, bool> GetManifestScope(winrt::Microsoft::Management::Deployment::PackageInstallScope scope)
    {
        switch (scope)
        {
        case winrt::Microsoft::Management::Deployment::PackageInstallScope::Any:
            return std::make_pair(::AppInstaller::Manifest::ScopeEnum::Unknown, false);
        case winrt::Microsoft::Management::Deployment::PackageInstallScope::User:
            return std::make_pair(::AppInstaller::Manifest::ScopeEnum::User, false);
        case winrt::Microsoft::Management::Deployment::PackageInstallScope::System:
            return std::make_pair(::AppInstaller::Manifest::ScopeEnum::Machine, false);
        case winrt::Microsoft::Management::Deployment::PackageInstallScope::UserOrUnknown:
            return std::make_pair(::AppInstaller::Manifest::ScopeEnum::User, true);
        case winrt::Microsoft::Management::Deployment::PackageInstallScope::SystemOrUnknown:
            return std::make_pair(::AppInstaller::Manifest::ScopeEnum::Machine, true);
        }

        return std::make_pair(::AppInstaller::Manifest::ScopeEnum::Unknown, false);
    }

    winrt::Microsoft::Management::Deployment::PackageInstallerType GetDeploymentInstallerType(::AppInstaller::Manifest::InstallerTypeEnum installerType)
    {
        switch (installerType)
        {
        case ::AppInstaller::Manifest::InstallerTypeEnum::Burn:
            return Microsoft::Management::Deployment::PackageInstallerType::Burn;
        case ::AppInstaller::Manifest::InstallerTypeEnum::Exe:
            return Microsoft::Management::Deployment::PackageInstallerType::Exe;
        case ::AppInstaller::Manifest::InstallerTypeEnum::Inno:
            return Microsoft::Management::Deployment::PackageInstallerType::Inno;
        case ::AppInstaller::Manifest::InstallerTypeEnum::Msi:
            return Microsoft::Management::Deployment::PackageInstallerType::Msi;
        case ::AppInstaller::Manifest::InstallerTypeEnum::Msix:
            return Microsoft::Management::Deployment::PackageInstallerType::Msix;
        case ::AppInstaller::Manifest::InstallerTypeEnum::MSStore:
            return Microsoft::Management::Deployment::PackageInstallerType::MSStore;
        case ::AppInstaller::Manifest::InstallerTypeEnum::Nullsoft:
            return Microsoft::Management::Deployment::PackageInstallerType::Nullsoft;
        case ::AppInstaller::Manifest::InstallerTypeEnum::Portable:
            return Microsoft::Management::Deployment::PackageInstallerType::Portable;
        case ::AppInstaller::Manifest::InstallerTypeEnum::Wix:
            return Microsoft::Management::Deployment::PackageInstallerType::Wix;
        case ::AppInstaller::Manifest::InstallerTypeEnum::Zip:
            return Microsoft::Management::Deployment::PackageInstallerType::Zip;
        case ::AppInstaller::Manifest::InstallerTypeEnum::Unknown:
            return Microsoft::Management::Deployment::PackageInstallerType::Unknown;
        }

        return Microsoft::Management::Deployment::PackageInstallerType::Unknown;
    }

    ::AppInstaller::Manifest::InstallerTypeEnum GetManifestInstallerType(winrt::Microsoft::Management::Deployment::PackageInstallerType installerType)
    {
        switch (installerType)
        {
        case Microsoft::Management::Deployment::PackageInstallerType::Burn:
            return ::AppInstaller::Manifest::InstallerTypeEnum::Burn;
        case Microsoft::Management::Deployment::PackageInstallerType::Exe:
            return ::AppInstaller::Manifest::InstallerTypeEnum::Exe;
        case Microsoft::Management::Deployment::PackageInstallerType::Inno:
            return ::AppInstaller::Manifest::InstallerTypeEnum::Inno;
        case Microsoft::Management::Deployment::PackageInstallerType::Msi:
            return ::AppInstaller::Manifest::InstallerTypeEnum::Msi;
        case Microsoft::Management::Deployment::PackageInstallerType::Msix:
            return ::AppInstaller::Manifest::InstallerTypeEnum::Msix;
        case Microsoft::Management::Deployment::PackageInstallerType::MSStore:
            return ::AppInstaller::Manifest::InstallerTypeEnum::MSStore;
        case Microsoft::Management::Deployment::PackageInstallerType::Nullsoft:
            return ::AppInstaller::Manifest::InstallerTypeEnum::Nullsoft;
        case Microsoft::Management::Deployment::PackageInstallerType::Portable:
            return ::AppInstaller::Manifest::InstallerTypeEnum::Portable;
        case Microsoft::Management::Deployment::PackageInstallerType::Wix:
            return ::AppInstaller::Manifest::InstallerTypeEnum::Wix;
        case Microsoft::Management::Deployment::PackageInstallerType::Zip:
            return ::AppInstaller::Manifest::InstallerTypeEnum::Zip;
        case Microsoft::Management::Deployment::PackageInstallerType::Unknown:
            return ::AppInstaller::Manifest::InstallerTypeEnum::Unknown;
        }

        return ::AppInstaller::Manifest::InstallerTypeEnum::Unknown;
    }

    winrt::Microsoft::Management::Deployment::PackageInstallerScope GetDeploymentInstallerScope(::AppInstaller::Manifest::ScopeEnum installerScope)
    {
        switch (installerScope)
        {
        case ::AppInstaller::Manifest::ScopeEnum::User:
            return Microsoft::Management::Deployment::PackageInstallerScope::User;
        case ::AppInstaller::Manifest::ScopeEnum::Machine:
            return Microsoft::Management::Deployment::PackageInstallerScope::System;
        case ::AppInstaller::Manifest::ScopeEnum::Unknown:
            return Microsoft::Management::Deployment::PackageInstallerScope::Unknown;
        }

        return Microsoft::Management::Deployment::PackageInstallerScope::Unknown;
    }

    ::AppInstaller::Manifest::ScopeEnum GetManifestUninstallScope(winrt::Microsoft::Management::Deployment::PackageUninstallScope scope)
    {
        switch (scope)
        {
        case winrt::Microsoft::Management::Deployment::PackageUninstallScope::Any:
            return ::AppInstaller::Manifest::ScopeEnum::Unknown;
        case winrt::Microsoft::Management::Deployment::PackageUninstallScope::User:
            return ::AppInstaller::Manifest::ScopeEnum::User;
        case winrt::Microsoft::Management::Deployment::PackageUninstallScope::System:
            return ::AppInstaller::Manifest::ScopeEnum::Machine;
        }

        return ::AppInstaller::Manifest::ScopeEnum::Unknown;
    }

    ::AppInstaller::Manifest::ScopeEnum GetManifestRepairScope(winrt::Microsoft::Management::Deployment::PackageRepairScope scope)
    {
        switch (scope)
        {
        case winrt::Microsoft::Management::Deployment::PackageRepairScope::Any:
            return ::AppInstaller::Manifest::ScopeEnum::Unknown;
        case winrt::Microsoft::Management::Deployment::PackageRepairScope::User:
            return ::AppInstaller::Manifest::ScopeEnum::User;
        case winrt::Microsoft::Management::Deployment::PackageRepairScope::System:
            return ::AppInstaller::Manifest::ScopeEnum::Machine;
        }

        return ::AppInstaller::Manifest::ScopeEnum::Unknown;
    }

    winrt::Microsoft::Management::Deployment::ElevationRequirement GetDeploymentElevationRequirement(::AppInstaller::Manifest::ElevationRequirementEnum elevationRequirement)
    {
        switch (elevationRequirement)
        {
        case ::AppInstaller::Manifest::ElevationRequirementEnum::ElevationRequired:
            return Microsoft::Management::Deployment::ElevationRequirement::ElevationRequired;
        case ::AppInstaller::Manifest::ElevationRequirementEnum::ElevationProhibited:
            return Microsoft::Management::Deployment::ElevationRequirement::ElevationProhibited;
        case ::AppInstaller::Manifest::ElevationRequirementEnum::ElevatesSelf:
            return Microsoft::Management::Deployment::ElevationRequirement::ElevatesSelf;
        case ::AppInstaller::Manifest::ElevationRequirementEnum::Unknown:
            return Microsoft::Management::Deployment::ElevationRequirement::Unknown;
        }

        return Microsoft::Management::Deployment::ElevationRequirement::Unknown;
    }

    winrt::Microsoft::Management::Deployment::IconFileType GetDeploymentIconFileType(::AppInstaller::Manifest::IconFileTypeEnum iconFileType)
    {
        switch (iconFileType)
        {
        case ::AppInstaller::Manifest::IconFileTypeEnum::Ico:
            return Microsoft::Management::Deployment::IconFileType::Ico;
        case ::AppInstaller::Manifest::IconFileTypeEnum::Jpeg:
            return Microsoft::Management::Deployment::IconFileType::Jpeg;
        case ::AppInstaller::Manifest::IconFileTypeEnum::Png:
            return Microsoft::Management::Deployment::IconFileType::Png;
        }

        return Microsoft::Management::Deployment::IconFileType::Unknown;
    }

    winrt::Microsoft::Management::Deployment::IconResolution GetDeploymentIconResolution(::AppInstaller::Manifest::IconResolutionEnum iconResolution)
    {
        switch (iconResolution)
        {
        case ::AppInstaller::Manifest::IconResolutionEnum::Custom:
            return Microsoft::Management::Deployment::IconResolution::Custom;
        case ::AppInstaller::Manifest::IconResolutionEnum::Square16:
            return Microsoft::Management::Deployment::IconResolution::Square16;
        case ::AppInstaller::Manifest::IconResolutionEnum::Square20:
            return Microsoft::Management::Deployment::IconResolution::Square20;
        case ::AppInstaller::Manifest::IconResolutionEnum::Square24:
            return Microsoft::Management::Deployment::IconResolution::Square24;
        case ::AppInstaller::Manifest::IconResolutionEnum::Square30:
            return Microsoft::Management::Deployment::IconResolution::Square30;
        case ::AppInstaller::Manifest::IconResolutionEnum::Square32:
            return Microsoft::Management::Deployment::IconResolution::Square32;
        case ::AppInstaller::Manifest::IconResolutionEnum::Square36:
            return Microsoft::Management::Deployment::IconResolution::Square36;
        case ::AppInstaller::Manifest::IconResolutionEnum::Square40:
            return Microsoft::Management::Deployment::IconResolution::Square40;
        case ::AppInstaller::Manifest::IconResolutionEnum::Square48:
            return Microsoft::Management::Deployment::IconResolution::Square48;
        case ::AppInstaller::Manifest::IconResolutionEnum::Square60:
            return Microsoft::Management::Deployment::IconResolution::Square60;
        case ::AppInstaller::Manifest::IconResolutionEnum::Square64:
            return Microsoft::Management::Deployment::IconResolution::Square64;
        case ::AppInstaller::Manifest::IconResolutionEnum::Square72:
            return Microsoft::Management::Deployment::IconResolution::Square72;
        case ::AppInstaller::Manifest::IconResolutionEnum::Square80:
            return Microsoft::Management::Deployment::IconResolution::Square80;
        case ::AppInstaller::Manifest::IconResolutionEnum::Square96:
            return Microsoft::Management::Deployment::IconResolution::Square96;
        case ::AppInstaller::Manifest::IconResolutionEnum::Square256:
            return Microsoft::Management::Deployment::IconResolution::Square256;
        }

        return Microsoft::Management::Deployment::IconResolution::Custom;
    }

    winrt::Microsoft::Management::Deployment::IconTheme GetDeploymentIconTheme(::AppInstaller::Manifest::IconThemeEnum iconTheme)
    {
        switch (iconTheme)
        {
        case ::AppInstaller::Manifest::IconThemeEnum::Default:
            return Microsoft::Management::Deployment::IconTheme::Default;
        case ::AppInstaller::Manifest::IconThemeEnum::Light:
            return Microsoft::Management::Deployment::IconTheme::Light;
        case ::AppInstaller::Manifest::IconThemeEnum::Dark:
            return Microsoft::Management::Deployment::IconTheme::Dark;
        case ::AppInstaller::Manifest::IconThemeEnum::HighContrast:
            return Microsoft::Management::Deployment::IconTheme::HighContrast;
        }

        return Microsoft::Management::Deployment::IconTheme::Unknown;
    }

    winrt::Microsoft::Management::Deployment::AuthenticationType GetDeploymentAuthenticationType(::AppInstaller::Authentication::AuthenticationType authType)
    {
        switch (authType)
        {
        case ::AppInstaller::Authentication::AuthenticationType::None:
            return Microsoft::Management::Deployment::AuthenticationType::None;
        case ::AppInstaller::Authentication::AuthenticationType::MicrosoftEntraId:
            return Microsoft::Management::Deployment::AuthenticationType::MicrosoftEntraId;
        case ::AppInstaller::Authentication::AuthenticationType::MicrosoftEntraIdForAzureBlobStorage:
            return Microsoft::Management::Deployment::AuthenticationType::MicrosoftEntraIdForAzureBlobStorage;
        }

        return Microsoft::Management::Deployment::AuthenticationType::Unknown;
    }

    ::AppInstaller::Authentication::AuthenticationMode GetAuthenticationMode(winrt::Microsoft::Management::Deployment::AuthenticationMode authMode)
    {
        switch (authMode)
        {
        case winrt::Microsoft::Management::Deployment::AuthenticationMode::Interactive:
            return ::AppInstaller::Authentication::AuthenticationMode::Interactive;
        case winrt::Microsoft::Management::Deployment::AuthenticationMode::SilentPreferred:
            return ::AppInstaller::Authentication::AuthenticationMode::SilentPreferred;
        case winrt::Microsoft::Management::Deployment::AuthenticationMode::Silent:
            return ::AppInstaller::Authentication::AuthenticationMode::Silent;
        }

        return ::AppInstaller::Authentication::AuthenticationMode::Unknown;
    }

    ::AppInstaller::Authentication::AuthenticationArguments GetAuthenticationArguments(winrt::Microsoft::Management::Deployment::AuthenticationArguments authArgs)
    {
        ::AppInstaller::Authentication::AuthenticationArguments result;
        result.Mode = ::AppInstaller::Authentication::AuthenticationMode::Silent; // Default to silent for com invocations.

        if (authArgs)
        {
            result.Mode = GetAuthenticationMode(authArgs.AuthenticationMode());
            result.AuthenticationAccount = ::AppInstaller::Utility::ConvertToUTF8(authArgs.AuthenticationAccount());
        }

        return result;
    }

    AddPackageCatalogStatus GetAddPackageCatalogOperationStatus(winrt::hresult hresult)
    {
        switch (hresult)
        {
        case APPINSTALLER_CLI_ERROR_AUTHENTICATION_TYPE_NOT_SUPPORTED:
            return AddPackageCatalogStatus::AuthenticationError;
        case APPINSTALLER_CLI_ERROR_SOURCE_NOT_SECURE:
        case APPINSTALLER_CLI_ERROR_INVALID_SOURCE_TYPE:
        case APPINSTALLER_CLI_ERROR_SOURCE_NOT_REMOTE:
        case APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS:
        case APPINSTALLER_CLI_ERROR_SOURCE_ARG_ALREADY_EXISTS:
            return AddPackageCatalogStatus::InvalidOptions;
        default:
            return HandleCommonCatalogOperationStatus<AddPackageCatalogStatus>(hresult);
        }
    }

    RemovePackageCatalogStatus GetRemovePackageCatalogOperationStatus(winrt::hresult hresult)
    {
        switch (hresult)
        {
        case APPINSTALLER_CLI_ERROR_SOURCE_NAME_DOES_NOT_EXIST:
            return RemovePackageCatalogStatus::InvalidOptions;
        case APPINSTALLER_CLI_ERROR_INVALID_SOURCE_TYPE:
            return RemovePackageCatalogStatus::CatalogError;
        default:
            return HandleCommonCatalogOperationStatus<RemovePackageCatalogStatus>(hresult);
        }
    }

}
