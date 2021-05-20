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
        hstring Name();
        hstring Version();
        hstring Channel();
        winrt::Windows::Foundation::Collections::IVectorView<hstring> PackageFamilyNames();
        winrt::Windows::Foundation::Collections::IVectorView<hstring> ProductCodes();
        winrt::Microsoft::Management::Deployment::AppCatalogReference AppCatalogReference();
    private:
        winrt::Microsoft::Management::Deployment::AppCatalogReference m_appCatalogReference{ nullptr };
        std::shared_ptr<::AppInstaller::Repository::IPackageVersion> m_packageVersion;
        Windows::Foundation::Collections::IVector<hstring> m_packageFamilyNames{ winrt::single_threaded_vector<hstring>() };
        Windows::Foundation::Collections::IVector<hstring> m_productCodes{ winrt::single_threaded_vector<hstring>() };
    };
}
