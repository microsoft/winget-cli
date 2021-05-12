#pragma once
#include "CatalogPackage.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid("d04c6127-b65a-4ec0-a3a1-5af39945544c")]
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

        void Initialize(std::shared_ptr<::AppInstaller::Repository::IPackage> package);
    private:
        std::shared_ptr<::AppInstaller::Repository::IPackage> m_package;
        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::PackageVersionId> m_availableVersions{ winrt::single_threaded_vector<Microsoft::Management::Deployment::PackageVersionId>() };
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
