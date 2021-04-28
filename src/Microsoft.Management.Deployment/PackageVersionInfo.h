#pragma once
#include "PackageVersionInfo.g.h"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageVersionInfo : PackageVersionInfoT<PackageVersionInfo>
    {
        PackageVersionInfo() = default;

        hstring GetMetadata(Microsoft::Management::Deployment::PackageVersionMetadata const& metadataType);
        hstring Id();
        hstring Name();
        hstring AppCatalogIdentifier();
        hstring AppCatalogName();
        hstring Version();
        hstring Channel();
        Windows::Foundation::Collections::IVectorView<hstring> PackageFamilyName();
        Windows::Foundation::Collections::IVectorView<hstring> ProductCode();
        Microsoft::Management::Deployment::AppCatalog AppCatalog();
    };
}
