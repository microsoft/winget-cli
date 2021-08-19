// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageVersionInfo.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageVersionInfo : PackageVersionInfoT<PackageVersionInfo>
    {
        PackageVersionInfo() = default;
        void Initialize(std::shared_ptr<::AppInstaller::Repository::IPackageVersion> packageVersion);

        hstring GetMetadata(winrt::Microsoft::Management::Deployment::PackageVersionMetadataField const& metadataField);
        hstring Id();
        hstring DisplayName();
        hstring Version();
        hstring Channel();
        winrt::Windows::Foundation::Collections::IVectorView<hstring> PackageFamilyNames();
        winrt::Windows::Foundation::Collections::IVectorView<hstring> ProductCodes();
        winrt::Microsoft::Management::Deployment::PackageCatalog PackageCatalog();
        std::shared_ptr<::AppInstaller::Repository::IPackageVersion> GetRepositoryPackageVersion() { return m_packageVersion; }
    private:
        winrt::Microsoft::Management::Deployment::PackageCatalog m_packageCatalog{ nullptr };
        std::shared_ptr<::AppInstaller::Repository::IPackageVersion> m_packageVersion;
        Windows::Foundation::Collections::IVector<hstring> m_packageFamilyNames{ nullptr };
        Windows::Foundation::Collections::IVector<hstring> m_productCodes{ nullptr };
    };
}
