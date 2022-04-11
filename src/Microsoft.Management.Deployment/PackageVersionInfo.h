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
        winrt::Microsoft::Management::Deployment::CompareResult CompareToVersion(hstring versionString);
        // Contract version 4
        bool HasApplicableInstaller(InstallOptions options);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::PackageCatalog m_packageCatalog{ nullptr };
        std::shared_ptr<::AppInstaller::Repository::IPackageVersion> m_packageVersion;
        Windows::Foundation::Collections::IVector<hstring> m_packageFamilyNames{ nullptr };
        Windows::Foundation::Collections::IVector<hstring> m_productCodes{ nullptr };
#endif
    };
}
