// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "CreateCompositePackageCatalogOptions.g.h"
#include "Public/ComClsids.h"
#include <winget/ModuleCountBase.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid(WINGET_OUTOFPROC_COM_CLSID_CreateCompositePackageCatalogOptions)]
    struct CreateCompositePackageCatalogOptions : CreateCompositePackageCatalogOptionsT<CreateCompositePackageCatalogOptions>
    {
        CreateCompositePackageCatalogOptions() = default;

        winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageCatalogReference> Catalogs();
        winrt::Microsoft::Management::Deployment::CompositeSearchBehavior CompositeSearchBehavior();
        void CompositeSearchBehavior(winrt::Microsoft::Management::Deployment::CompositeSearchBehavior const& value);
        winrt::Microsoft::Management::Deployment::PackageInstallScope InstalledScope();
        void InstalledScope(winrt::Microsoft::Management::Deployment::PackageInstallScope const& value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageCatalogReference> m_catalogs{ winrt::single_threaded_vector<Microsoft::Management::Deployment::PackageCatalogReference>() };
        winrt::Microsoft::Management::Deployment::CompositeSearchBehavior m_compositeSearchBehavior = winrt::Microsoft::Management::Deployment::CompositeSearchBehavior::RemotePackagesFromAllCatalogs;
        winrt::Microsoft::Management::Deployment::PackageInstallScope m_installedScope = winrt::Microsoft::Management::Deployment::PackageInstallScope::Any;
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct CreateCompositePackageCatalogOptions :
        CreateCompositePackageCatalogOptionsT<CreateCompositePackageCatalogOptions, implementation::CreateCompositePackageCatalogOptions>,
        AppInstaller::WinRT::ModuleCountBase
    {
    };
}
#endif
