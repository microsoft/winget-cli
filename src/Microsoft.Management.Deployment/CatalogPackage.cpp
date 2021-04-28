#include "pch.h"
#include "CatalogPackage.h"
#include "CatalogPackage.g.cpp"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


namespace winrt::Microsoft::Management::Deployment::implementation
{
    Microsoft::Management::Deployment::CatalogPackage CatalogPackage::TryCreateFromManifest(hstring const& manifestPath)
    {
        throw hresult_not_implemented();
    }
    hstring CatalogPackage::Id()
    {
        throw hresult_not_implemented();
    }
    hstring CatalogPackage::Name()
    {
        throw hresult_not_implemented();
    }
    Microsoft::Management::Deployment::PackageVersionInfo CatalogPackage::InstalledVersion()
    {
        throw hresult_not_implemented();
    }
    Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::PackageVersionId> CatalogPackage::AvailableVersions()
    {
        throw hresult_not_implemented();
    }
    Microsoft::Management::Deployment::PackageVersionInfo CatalogPackage::LatestAvailableVersion()
    {
        throw hresult_not_implemented();
    }
    Microsoft::Management::Deployment::PackageVersionInfo CatalogPackage::GetAvailableVersion(Microsoft::Management::Deployment::PackageVersionId const& versionKey)
    {
        throw hresult_not_implemented();
    }
    bool CatalogPackage::IsUpdateAvailable()
    {
        throw hresult_not_implemented();
    }
    bool CatalogPackage::IsInstalling()
    {
        throw hresult_not_implemented();
    }
}
