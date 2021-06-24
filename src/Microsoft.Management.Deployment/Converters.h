// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageMatchFilter.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Microsoft::Management::Deployment::PackageMatchField GetDeploymentMatchField(::AppInstaller::Repository::PackageMatchField field);
    ::AppInstaller::Repository::PackageMatchField GetRepositoryMatchField(winrt::Microsoft::Management::Deployment::PackageMatchField field);
    winrt::Microsoft::Management::Deployment::PackageFieldMatchOption GetDeploymentMatchOption(::AppInstaller::Repository::MatchType type);
    ::AppInstaller::Repository::MatchType GetRepositoryMatchType(winrt::Microsoft::Management::Deployment::PackageFieldMatchOption option);
    ::AppInstaller::Repository::CompositeSearchBehavior GetRepositoryCompositeSearchBehavior(winrt::Microsoft::Management::Deployment::CompositeSearchBehavior searchBehavior);
    ::AppInstaller::Repository::PackageVersionMetadata GetRepositoryPackageVersionMetadata(winrt::Microsoft::Management::Deployment::PackageVersionMetadataField packageVersionMetadataField);
    winrt::Microsoft::Management::Deployment::InstallResultStatus GetInstallResultStatus(::AppInstaller::CLI::Workflow::ExecutionStage executionStage, winrt::hresult hresult);
    winrt::Microsoft::Management::Deployment::FindPackagesResultStatus FindPackagesResultStatus(winrt::hresult hresult);
    bool IsLocalPackageCatalog(winrt::Microsoft::Management::Deployment::PackageCatalogInfo info);
}