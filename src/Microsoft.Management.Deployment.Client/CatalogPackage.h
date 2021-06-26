// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "CatalogPackage.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct CatalogPackage : CatalogPackageT<CatalogPackage>
    {
        CatalogPackage() = default;

        hstring Id();
        hstring Name();
        winrt::Microsoft::Management::Deployment::PackageVersionInfo InstalledVersion();
        winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::PackageVersionId> AvailableVersions();
        winrt::Microsoft::Management::Deployment::PackageVersionInfo DefaultInstallVersion();
        winrt::Microsoft::Management::Deployment::PackageVersionInfo GetPackageVersionInfo(winrt::Microsoft::Management::Deployment::PackageVersionId const& versionKey);
        bool IsUpdateAvailable();

        winrt::Microsoft::Management::Deployment::CatalogPackage GetServerPackage();
    private:
        winrt::Microsoft::Management::Deployment::CatalogPackage m_catalogPackage{ nullptr };
    };
}
