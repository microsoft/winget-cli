// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageVersionInfo.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageVersionInfo : PackageVersionInfoT<PackageVersionInfo>
    {
        PackageVersionInfo() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(std::shared_ptr<::AppInstaller::Repository::IPackageVersion> packageVersion);
        std::shared_ptr<::AppInstaller::Repository::IPackageVersion> GetRepositoryPackageVersion();
#endif

        hstring GetMetadata(winrt::Microsoft::Management::Deployment::PackageVersionMetadataField const& metadataField);
        hstring Id();
        hstring DisplayName();
        hstring Publisher();
        hstring Version();
        hstring Channel();
        winrt::Windows::Foundation::Collections::IVectorView<hstring> PackageFamilyNames();
        winrt::Windows::Foundation::Collections::IVectorView<hstring> ProductCodes();
        winrt::Microsoft::Management::Deployment::PackageCatalog PackageCatalog();
        winrt::Microsoft::Management::Deployment::CompareResult CompareToVersion(const hstring& versionString);
        // Contract 4.0
        bool HasApplicableInstaller(InstallOptions options);
        // Contract 6.0
        winrt::Microsoft::Management::Deployment::CatalogPackageMetadata GetCatalogPackageMetadata();
        winrt::Microsoft::Management::Deployment::CatalogPackageMetadata GetCatalogPackageMetadata(const hstring& preferredLocale);
        winrt::Microsoft::Management::Deployment::PackageInstallerInfo GetApplicableInstaller(InstallOptions options);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::PackageCatalog m_packageCatalog{ nullptr };
        std::shared_ptr<::AppInstaller::Repository::IPackageVersion> m_packageVersion;
        Windows::Foundation::Collections::IVector<hstring> m_packageFamilyNames{ nullptr };
        Windows::Foundation::Collections::IVector<hstring> m_productCodes{ nullptr };
#endif
    };
}
