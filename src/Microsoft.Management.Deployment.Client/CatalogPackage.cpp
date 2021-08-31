// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <CatalogPackage.h>
#include "CatalogPackage.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    hstring CatalogPackage::Id()
    {
        throw hresult_not_implemented();
    }
    hstring CatalogPackage::Name()
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::PackageVersionInfo CatalogPackage::InstalledVersion()
    {
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::PackageVersionId> CatalogPackage::AvailableVersions()
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::PackageVersionInfo CatalogPackage::DefaultInstallVersion()
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::PackageVersionInfo CatalogPackage::GetPackageVersionInfo(winrt::Microsoft::Management::Deployment::PackageVersionId const&)
    {
        throw hresult_not_implemented();
    }
    bool CatalogPackage::IsUpdateAvailable()
    {
        throw hresult_not_implemented();
    }
}
