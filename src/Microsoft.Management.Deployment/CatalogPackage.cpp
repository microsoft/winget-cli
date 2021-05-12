#include "pch.h"
#include <AppInstallerRepositorySource.h>
#include <AppInstallerRepositorySearch.h>
#include "CatalogPackage.h"
#include "CatalogPackage.g.cpp"
#include "AppCatalog.h"
#include "PackageVersionInfo.h"
#include "PackageVersionId.h"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    CatalogPackage::CatalogPackage(std::shared_ptr<::AppInstaller::Repository::IPackage> package)
    {
        m_package = package;
    }
    void CatalogPackage::Initialize(std::shared_ptr<::AppInstaller::Repository::IPackage> package)
    {
        m_package = package;
    }
    Microsoft::Management::Deployment::CatalogPackage CatalogPackage::TryCreateFromManifest(hstring const& manifestPath)
    {
        throw hresult_not_implemented();
    }
    hstring CatalogPackage::Id()
    {
        return winrt::to_hstring(m_package->GetProperty(::AppInstaller::Repository::PackageProperty::Id).get());
    }
    hstring CatalogPackage::Name()
    {
        hstring name = winrt::to_hstring(m_package->GetProperty(::AppInstaller::Repository::PackageProperty::Name).get());
        return name;
    }
    Microsoft::Management::Deployment::PackageVersionInfo CatalogPackage::InstalledVersion()
    {
        if (!m_installedVersion)
        {
            //m_installedVersion = winrt::make<winrt::Microsoft::Management::Deployment::implementation::PackageVersionInfo>(m_package.get()->GetInstalledVersion());
            auto installedVersionImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageVersionInfo>>();
            installedVersionImpl->Initialize(m_package.get()->GetInstalledVersion());
            m_installedVersion = *installedVersionImpl;
        }
        return m_installedVersion;
    }
    Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::PackageVersionId> CatalogPackage::AvailableVersions()
    {
        if (m_availableVersions.Size() == 0)
        {
            std::vector<::AppInstaller::Repository::PackageVersionKey> keys = m_package.get()->GetAvailableVersionKeys();
            for (int i = 0; i < keys.size(); ++i)
            {
                //Microsoft::Management::Deployment::PackageVersionId packageVersionId{ nullptr };
                //packageVersionId = winrt::make<winrt::Microsoft::Management::Deployment::implementation::PackageVersionId>(keys[i]);
                //m_availableVersions.Append(packageVersionId);
                auto packageVersionId = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageVersionId>>();
                packageVersionId->Initialize(keys[i]);
                m_availableVersions.Append(*packageVersionId);
            }
        }
        return m_availableVersions.GetView();
    }
    Microsoft::Management::Deployment::PackageVersionInfo CatalogPackage::LatestAvailableVersion()
    {
        if (!m_latestAvailableVersion)
        {
            //m_latestAvailableVersion = winrt::make<winrt::Microsoft::Management::Deployment::implementation::PackageVersionInfo>(m_package.get()->GetLatestAvailableVersion());
            auto latestVersionImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageVersionInfo>>();
            latestVersionImpl->Initialize(m_package.get()->GetLatestAvailableVersion());
            m_latestAvailableVersion = *latestVersionImpl;
        }
        return m_latestAvailableVersion;
    }
    Microsoft::Management::Deployment::PackageVersionInfo CatalogPackage::GetAvailableVersion(Microsoft::Management::Deployment::PackageVersionId const& versionKey)
    {
        ::AppInstaller::Repository::PackageVersionKey internalVersionKey(winrt::to_string(versionKey.AppCatalogId()), winrt::to_string(versionKey.Version()), winrt::to_string(versionKey.Channel()));
        //Microsoft::Management::Deployment::PackageVersionInfo packageVersionInfo{ nullptr };
        //packageVersionInfo = winrt::make<winrt::Microsoft::Management::Deployment::implementation::PackageVersionInfo>(m_package.get()->GetAvailableVersion(internalVersionKey));
        //return packageVersionInfo;
        auto packageVersionInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageVersionInfo>>();
        packageVersionInfo->Initialize(m_package.get()->GetAvailableVersion(internalVersionKey));
        return *packageVersionInfo;
    }
    bool CatalogPackage::IsUpdateAvailable()
    {
        return m_package->IsUpdateAvailable();
    }
    bool CatalogPackage::IsInstalling()
    {
        //TODO - installing source does not exist yet.
        return false;
    }
    CoCreatableCppWinRtClass(CatalogPackage);
}
