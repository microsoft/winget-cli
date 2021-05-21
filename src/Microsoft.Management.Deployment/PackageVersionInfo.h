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
        winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageCatalogReference();
    private:
        winrt::Microsoft::Management::Deployment::PackageCatalogReference m_packageCatalogReference{ nullptr };
        std::shared_ptr<::AppInstaller::Repository::IPackageVersion> m_packageVersion;
        Windows::Foundation::Collections::IVector<hstring> m_packageFamilyNames{ winrt::single_threaded_vector<hstring>() };
        Windows::Foundation::Collections::IVector<hstring> m_productCodes{ winrt::single_threaded_vector<hstring>() };
    };
}
