#include "pch.h"
#include <AppInstallerRepositorySource.h>
#include <AppInstallerRepositorySearch.h>
#include "CatalogPackage.h"
#include "CatalogPackage.g.cpp"
#include "PackageVersionInfo.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    CatalogPackage::CatalogPackage(std::shared_ptr<::AppInstaller::Repository::IPackage> package)
        : m_package(std::move(package))
    {
    }
    Microsoft::Management::Deployment::CatalogPackage CatalogPackage::TryCreateFromManifest(hstring const& manifestPath)
    {
        throw hresult_not_implemented();
    }
    hstring CatalogPackage::Id()
    {
        return  winrt::to_hstring(m_package->GetProperty(::AppInstaller::Repository::PackageProperty::Id).get());
    }
    hstring CatalogPackage::Name()
    {
        return  winrt::to_hstring(m_package->GetProperty(::AppInstaller::Repository::PackageProperty::Name).get());
    }
    Microsoft::Management::Deployment::PackageVersionInfo CatalogPackage::InstalledVersion()
    {
        if (!m_installedVersion)
        {
            Microsoft::Management::Deployment::PackageVersionInfo packageVersionInfo{ nullptr };
            packageVersionInfo = winrt::make<winrt::Microsoft::Management::Deployment::implementation::PackageVersionInfo>(m_package.get()->GetLatestAvailableVersion());
        }
        return m_installedVersion;
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
        return m_package->IsUpdateAvailable();
    }
    bool CatalogPackage::IsInstalling()
    {
        return false;
    }
}
