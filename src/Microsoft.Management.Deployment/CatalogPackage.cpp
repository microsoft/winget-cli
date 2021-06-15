// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <mutex>
#include <AppInstallerRepositorySource.h>
#include <AppInstallerRepositorySearch.h>
#include "CatalogPackage.h"
#include "CatalogPackage.g.cpp"
#include "PackageCatalog.h"
#include "PackageVersionInfo.h"
#include "PackageVersionId.h"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void CatalogPackage::Initialize(
        std::shared_ptr<const ::AppInstaller::Repository::ISource> source,
        std::shared_ptr<::AppInstaller::Repository::IPackage> package)
    {
        m_source = std::move(source);
        m_package = std::move(package);
    }
    hstring CatalogPackage::Id()
    {
        return winrt::to_hstring(m_package->GetProperty(::AppInstaller::Repository::PackageProperty::Id).get());
    }
    hstring CatalogPackage::Name()
    {
        return winrt::to_hstring(m_package->GetProperty(::AppInstaller::Repository::PackageProperty::Name));
    }
    Microsoft::Management::Deployment::PackageVersionInfo CatalogPackage::InstalledVersion()
    {
        std::call_once(m_installedVersionOnceFlag,
            [&]()
            {
                std::shared_ptr<::AppInstaller::Repository::IPackageVersion> installedVersion = m_package.get()->GetInstalledVersion();
                if (installedVersion)
                {
                    auto installedVersionImpl = winrt::make_self<wil::details::module_count_wrapper<
                        winrt::Microsoft::Management::Deployment::implementation::PackageVersionInfo>>();
                    installedVersionImpl->Initialize(std::move(installedVersion));
                    m_installedVersion = *installedVersionImpl;
                }
            });
        return m_installedVersion;
    }
    Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::PackageVersionId> CatalogPackage::AvailableVersions()
    {
        std::call_once(m_availableVersionsOnceFlag,
            [&]()
            {
                // Vector hasn't been populated yet.
                for (auto const& versionKey : m_package.get()->GetAvailableVersionKeys())
                {
                    auto packageVersionId = winrt::make_self<wil::details::module_count_wrapper<
                        winrt::Microsoft::Management::Deployment::implementation::PackageVersionId>>();
                    packageVersionId->Initialize(versionKey);
                    m_availableVersions.Append(*packageVersionId);
                }
            });
        return m_availableVersions.GetView();
    }
    Microsoft::Management::Deployment::PackageVersionInfo CatalogPackage::DefaultInstallVersion()
    {
        std::call_once(m_defaultInstallVersionOnceFlag,
            [&]()
            {
                std::shared_ptr<::AppInstaller::Repository::IPackageVersion> latestVersion = m_package.get()->GetLatestAvailableVersion();
                if (latestVersion)
                {
                    // DefaultInstallVersion hasn't been created yet, create and populate it.
                    // DefaultInstallVersion is the LatestAvailableVersion of the internal package object.
                    auto latestVersionImpl = winrt::make_self<wil::details::module_count_wrapper<
                        winrt::Microsoft::Management::Deployment::implementation::PackageVersionInfo>>();
                    latestVersionImpl->Initialize(std::move(latestVersion));
                    m_defaultInstallVersion = *latestVersionImpl;
                }
            });
        return m_defaultInstallVersion;
    }
    Microsoft::Management::Deployment::PackageVersionInfo CatalogPackage::GetPackageVersionInfo(Microsoft::Management::Deployment::PackageVersionId const& versionKey)
    {
        winrt::Microsoft::Management::Deployment::PackageVersionInfo packageVersionInfo{ nullptr };

        ::AppInstaller::Repository::PackageVersionKey internalVersionKey(winrt::to_string(versionKey.PackageCatalogId()), winrt::to_string(versionKey.Version()), winrt::to_string(versionKey.Channel()));
        std::shared_ptr<::AppInstaller::Repository::IPackageVersion> availableVersion = m_package.get()->GetAvailableVersion(internalVersionKey);
        if (availableVersion)
        {
            auto packageVersionInfoImpl = winrt::make_self<wil::details::module_count_wrapper<
                winrt::Microsoft::Management::Deployment::implementation::PackageVersionInfo>>();
            packageVersionInfoImpl->Initialize(std::move(availableVersion));
            packageVersionInfo =*packageVersionInfoImpl;
        }
        return packageVersionInfo;
    }
    bool CatalogPackage::IsUpdateAvailable()
    {
        return m_package->IsUpdateAvailable();
    }
}
