#include "pch.h"
#include <AppInstallerErrors.h>
#include <AppInstallerRepositorySearch.h>
#include <AppInstallerRepositorySource.h>
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

    winrt::Microsoft::Management::Deployment::InstallResultStatus GetInstallResultStatus(winrt::hresult hresult)
    {
        // TODO: Error mapping and defining in the idl still needed for all the rest of the errors.
        winrt::Microsoft::Management::Deployment::InstallResultStatus resultStatus = winrt::Microsoft::Management::Deployment::InstallResultStatus::Ok;
        switch (hresult)
        {
        case(S_OK):
            resultStatus = winrt::Microsoft::Management::Deployment::InstallResultStatus::Ok;
            break;
        case APPINSTALLER_CLI_ERROR_INTERNAL_ERROR:
            resultStatus = winrt::Microsoft::Management::Deployment::InstallResultStatus::InternalError;
            break;
        case APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER:
            resultStatus = winrt::Microsoft::Management::Deployment::InstallResultStatus::InternalError;
            break;
        default:
            resultStatus = winrt::Microsoft::Management::Deployment::InstallResultStatus::InternalError;
            break;
        }
        return resultStatus;
    }
}