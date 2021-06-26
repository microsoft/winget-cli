// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CreateCompositePackageCatalogOptions.h"
#include "CreateCompositePackageCatalogOptions.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageCatalogReference> CreateCompositePackageCatalogOptions::Catalogs()
    {
        return m_createCompositePackageCatalogOptions.Catalogs();
    }
    winrt::Microsoft::Management::Deployment::CompositeSearchBehavior CreateCompositePackageCatalogOptions::CompositeSearchBehavior()
    {
        return m_createCompositePackageCatalogOptions.CompositeSearchBehavior();
    }
    void CreateCompositePackageCatalogOptions::CompositeSearchBehavior(winrt::Microsoft::Management::Deployment::CompositeSearchBehavior const& value)
    {
        m_createCompositePackageCatalogOptions.CompositeSearchBehavior(value);
    }
}
