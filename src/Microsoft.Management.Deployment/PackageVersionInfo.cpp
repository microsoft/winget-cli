#include "pch.h"
#include "PackageVersionInfo.h"
#include "PackageVersionInfo.g.cpp"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


namespace winrt::Microsoft::Management::Deployment::implementation
{
    hstring PackageVersionInfo::GetMetadata(Microsoft::Management::Deployment::PackageVersionMetadata const& metadataType)
    {
        throw hresult_not_implemented();
    }
    hstring PackageVersionInfo::Id()
    {
        throw hresult_not_implemented();
    }
    hstring PackageVersionInfo::Name()
    {
        throw hresult_not_implemented();
    }
    hstring PackageVersionInfo::AppCatalogIdentifier()
    {
        throw hresult_not_implemented();
    }
    hstring PackageVersionInfo::AppCatalogName()
    {
        throw hresult_not_implemented();
    }
    hstring PackageVersionInfo::Version()
    {
        throw hresult_not_implemented();
    }
    hstring PackageVersionInfo::Channel()
    {
        throw hresult_not_implemented();
    }
    Windows::Foundation::Collections::IVectorView<hstring> PackageVersionInfo::PackageFamilyName()
    {
        throw hresult_not_implemented();
    }
    Windows::Foundation::Collections::IVectorView<hstring> PackageVersionInfo::ProductCode()
    {
        throw hresult_not_implemented();
    }
    Microsoft::Management::Deployment::AppCatalog PackageVersionInfo::AppCatalog()
    {
        throw hresult_not_implemented();
    }
}
