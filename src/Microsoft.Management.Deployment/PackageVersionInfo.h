#pragma once
#include "PackageVersionInfo.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid("83559159-eb8d-4dae-bf59-a6c1dac0212d")]
    struct PackageVersionInfo : PackageVersionInfoT<PackageVersionInfo>
    {
        PackageVersionInfo() = default;
        PackageVersionInfo(std::shared_ptr<::AppInstaller::Repository::IPackageVersion> packageVersion);
        void Initialize(std::shared_ptr<::AppInstaller::Repository::IPackageVersion> packageVersion);

        hstring GetMetadata(Microsoft::Management::Deployment::PackageVersionMetadata const& metadataType);
        hstring Id();
        hstring Name();
        hstring Version();
        hstring Channel();
        Windows::Foundation::Collections::IVectorView<hstring> PackageFamilyName();
        Windows::Foundation::Collections::IVectorView<hstring> ProductCode();
        Microsoft::Management::Deployment::AppCatalog AppCatalog();
    private:
        Microsoft::Management::Deployment::AppCatalog m_appCatalog{ nullptr };
        std::shared_ptr<::AppInstaller::Repository::IPackageVersion> m_packageVersion;
        Windows::Foundation::Collections::IVector<hstring> m_packageFamilyNames{ winrt::single_threaded_vector<hstring>() };
        Windows::Foundation::Collections::IVector<hstring> m_productCodes{ winrt::single_threaded_vector<hstring>() };
    };
}
