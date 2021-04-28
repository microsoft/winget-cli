#pragma once
#include "CatalogPackage.g.h"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct CatalogPackage : CatalogPackageT<CatalogPackage>
    {
        CatalogPackage() = default;

        static Microsoft::Management::Deployment::CatalogPackage TryCreateFromManifest(hstring const& manifestPath);
        hstring Id();
        hstring Name();
        Microsoft::Management::Deployment::PackageVersionInfo InstalledVersion();
        Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::PackageVersionId> AvailableVersions();
        Microsoft::Management::Deployment::PackageVersionInfo LatestAvailableVersion();
        Microsoft::Management::Deployment::PackageVersionInfo GetAvailableVersion(Microsoft::Management::Deployment::PackageVersionId const& versionKey);
        bool IsUpdateAvailable();
        bool IsInstalling();
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct CatalogPackage : CatalogPackageT<CatalogPackage, implementation::CatalogPackage>
    {
    };
}
