// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageVersionInfo.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    // This local-only interface must not be added to WinMD or registered for marshaling.
    MIDL_INTERFACE("33b5886b-925c-4da5-b30d-e83e2132dd04")
    IPackageVersionInfoInternal : public ::IUnknown
    {
        virtual HRESULT STDMETHODCALLTYPE GetRepositoryPackageVersion(
            std::shared_ptr<::AppInstaller::Repository::IPackageVersion>* packageVersion) noexcept = 0;
    };

    struct PackageVersionInfo : PackageVersionInfoT<PackageVersionInfo, winrt::cloaked<IPackageVersionInfoInternal>>
#else
    struct PackageVersionInfo : PackageVersionInfoT<PackageVersionInfo>
#endif
    {
        PackageVersionInfo() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(std::shared_ptr<::AppInstaller::Repository::IPackageVersion> packageVersion);
        static std::shared_ptr<::AppInstaller::Repository::IPackageVersion> GetRepositoryPackageVersion(
            winrt::Microsoft::Management::Deployment::PackageVersionInfo const& packageVersionInfo);
        HRESULT STDMETHODCALLTYPE GetRepositoryPackageVersion(
            std::shared_ptr<::AppInstaller::Repository::IPackageVersion>* packageVersion) noexcept override;
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
