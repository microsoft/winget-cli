// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "CreateCompositePackageCatalogOptions.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid("526534B8-7E46-47C8-8416-B1685C327D37")]
    struct CreateCompositePackageCatalogOptions : CreateCompositePackageCatalogOptionsT<CreateCompositePackageCatalogOptions>
    {
        CreateCompositePackageCatalogOptions() = default;

        winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageCatalogReference> Catalogs();
        winrt::Microsoft::Management::Deployment::CompositeSearchBehavior CompositeSearchBehavior();
        void CompositeSearchBehavior(winrt::Microsoft::Management::Deployment::CompositeSearchBehavior const& value);
    private:
        winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageCatalogReference> m_catalogs{ winrt::single_threaded_vector<Microsoft::Management::Deployment::PackageCatalogReference>() };
        winrt::Microsoft::Management::Deployment::CompositeSearchBehavior m_compositeSearchBehavior = winrt::Microsoft::Management::Deployment::CompositeSearchBehavior::RemotePackagesFromAllCatalogs;
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct CreateCompositePackageCatalogOptions : CreateCompositePackageCatalogOptionsT<CreateCompositePackageCatalogOptions, implementation::CreateCompositePackageCatalogOptions>
    {
    };
}
