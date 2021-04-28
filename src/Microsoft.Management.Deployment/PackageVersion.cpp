#include "pch.h"
#include "PackageVersion.h"
#include "PackageVersion.g.cpp"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


namespace winrt::Microsoft::Management::Deployment::implementation
{
    hstring PackageVersion::GetMetadata(Microsoft::Management::Deployment::PackageVersionMetadata const& metadataType)
    {
        throw hresult_not_implemented();
    }
    hstring PackageVersion::Id()
    {
        throw hresult_not_implemented();
    }
    hstring PackageVersion::Name()
    {
        throw hresult_not_implemented();
    }
    hstring PackageVersion::AppCatalogIdentifier()
    {
        throw hresult_not_implemented();
    }
    hstring PackageVersion::AppCatalogName()
    {
        throw hresult_not_implemented();
    }
    hstring PackageVersion::Version()
    {
        throw hresult_not_implemented();
    }
    hstring PackageVersion::Channel()
    {
        throw hresult_not_implemented();
    }
    Windows::Foundation::Collections::IVectorView<hstring> PackageVersion::PackageFamilyName()
    {
        throw hresult_not_implemented();
    }
    Windows::Foundation::Collections::IVectorView<hstring> PackageVersion::ProductCode()
    {
        throw hresult_not_implemented();
    }
    Microsoft::Management::Deployment::AppCatalog PackageVersion::AppCatalog()
    {
        throw hresult_not_implemented();
    }
}
