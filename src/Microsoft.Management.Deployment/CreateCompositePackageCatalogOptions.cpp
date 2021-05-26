// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 6388 Allow CreateInstance.
#include <wil\cppwinrt_wrl.h>
// 4467 Allow use of uuid attribute for com object creation.
#include "CreateCompositePackageCatalogOptions.h"
#pragma warning( pop )
#include "CreateCompositePackageCatalogOptions.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageCatalog> CreateCompositePackageCatalogOptions::Catalogs()
    {
        return m_catalogs;
    }
    winrt::Microsoft::Management::Deployment::PackageCatalog CreateCompositePackageCatalogOptions::LocalPackageCatalog()
    {
        return m_localPackageCatalog;
    }
    void CreateCompositePackageCatalogOptions::LocalPackageCatalog(winrt::Microsoft::Management::Deployment::PackageCatalog const& value)
    {
        m_localPackageCatalog = value;
    }
    winrt::Microsoft::Management::Deployment::CompositeSearchBehavior CreateCompositePackageCatalogOptions::CompositeSearchBehavior()
    {
        return m_compositeSearchBehavior;
    }
    void CreateCompositePackageCatalogOptions::CompositeSearchBehavior(winrt::Microsoft::Management::Deployment::CompositeSearchBehavior const& value)
    {
        m_compositeSearchBehavior = value;
    }
    CoCreatableCppWinRtClass(CreateCompositePackageCatalogOptions);
}
