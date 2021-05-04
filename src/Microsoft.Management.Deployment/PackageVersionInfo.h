#pragma once
#include "PackageVersionInfo.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageVersionInfo : PackageVersionInfoT<PackageVersionInfo>
    {
        PackageVersionInfo() = default;
        PackageVersionInfo(std::shared_ptr<::AppInstaller::Repository::IPackageVersion> packageVersion);

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
