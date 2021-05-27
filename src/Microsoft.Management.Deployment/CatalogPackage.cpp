// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
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
        m_source = source;
        m_package = package;
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
        if (!m_installedVersion)
        {
            // InstalledVersion hasn't been created yet, create and populate it.
            std::shared_ptr<::AppInstaller::Repository::IPackageVersion> installedVersion = m_package.get()->GetInstalledVersion();
            if (installedVersion)
            {
                auto installedVersionImpl = winrt::make_self<wil::details::module_count_wrapper<
                    winrt::Microsoft::Management::Deployment::implementation::PackageVersionInfo>>();
                installedVersionImpl->Initialize(installedVersion);
                m_installedVersion = *installedVersionImpl;
            }
        }
        return m_installedVersion;
    }
    Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::PackageVersionId> CatalogPackage::AvailableVersions()
    {
        if (!m_availableVersions)
        {
            // Vector hasn't been created yet, create and populate it.
            auto availableVersions = winrt::single_threaded_vector<winrt::Microsoft::Management::Deployment::PackageVersionId>();
            std::vector<::AppInstaller::Repository::PackageVersionKey> keys = m_package.get()->GetAvailableVersionKeys();
            for (int i = 0; i < keys.size(); ++i)
            {
                auto packageVersionId = winrt::make_self<wil::details::module_count_wrapper<
                    winrt::Microsoft::Management::Deployment::implementation::PackageVersionId>>();
                packageVersionId->Initialize(keys[i]);
                availableVersions.Append(*packageVersionId);
            }
            m_availableVersions = availableVersions;
        }
        return m_availableVersions.GetView();
    }
    Microsoft::Management::Deployment::PackageVersionInfo CatalogPackage::DefaultInstallVersion()
    {
        if (!m_defaultInstallVersion)
        {
            std::shared_ptr<::AppInstaller::Repository::IPackageVersion> latestVersion = m_package.get()->GetLatestAvailableVersion();
            if (latestVersion)
            {
                // DefaultInstallVersion hasn't been created yet, create and populate it.
                // DefaultInstallVersion is the LatestAvailableVersion of the internal package object.
                auto latestVersionImpl = winrt::make_self<wil::details::module_count_wrapper<
                    winrt::Microsoft::Management::Deployment::implementation::PackageVersionInfo>>();
                latestVersionImpl->Initialize(latestVersion);
                m_defaultInstallVersion = *latestVersionImpl;
            }

        }
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
            packageVersionInfoImpl->Initialize(availableVersion);
            packageVersionInfo =*packageVersionInfoImpl;
        }
        return packageVersionInfo;
    }
    bool CatalogPackage::IsUpdateAvailable()
    {
        return m_package->IsUpdateAvailable();
    }
}
