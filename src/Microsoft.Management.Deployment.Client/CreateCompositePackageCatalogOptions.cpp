// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
#include <CreateCompositePackageCatalogOptions.h>
#include <Client.CreateCompositePackageCatalogOptions.h>
#pragma warning( pop )
#include "CreateCompositePackageCatalogOptions.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageCatalogReference> CreateCompositePackageCatalogOptions::Catalogs()
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::CompositeSearchBehavior CreateCompositePackageCatalogOptions::CompositeSearchBehavior()
    {
        throw hresult_not_implemented();
    }
    void CreateCompositePackageCatalogOptions::CompositeSearchBehavior(winrt::Microsoft::Management::Deployment::CompositeSearchBehavior const&)
    {
        throw hresult_not_implemented();
    }
}
