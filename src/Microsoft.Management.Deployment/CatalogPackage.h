#pragma once
#include "CatalogPackage.g.h"





namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct CatalogPackage : CatalogPackageT<CatalogPackage>
    {
        CatalogPackage() = default;
        CatalogPackage(std::shared_ptr<::AppInstaller::Repository::IPackage> package);

        static Microsoft::Management::Deployment::CatalogPackage TryCreateFromManifest(hstring const& manifestPath);
        hstring Id();
        hstring Name();
        Microsoft::Management::Deployment::PackageVersionInfo InstalledVersion();
        Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::PackageVersionId> AvailableVersions();
        Microsoft::Management::Deployment::PackageVersionInfo LatestAvailableVersion();
        Microsoft::Management::Deployment::PackageVersionInfo GetAvailableVersion(Microsoft::Management::Deployment::PackageVersionId const& versionKey);
        bool IsUpdateAvailable();
        bool IsInstalling();
    private:
        std::shared_ptr<::AppInstaller::Repository::IPackage> m_package;
        Microsoft::Management::Deployment::PackageVersionInfo m_installedVersion{ nullptr };
        Microsoft::Management::Deployment::PackageVersionInfo m_latestAvailableVersion{ nullptr };
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct CatalogPackage : CatalogPackageT<CatalogPackage, implementation::CatalogPackage>
    {
    };
}
