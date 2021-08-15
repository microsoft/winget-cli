// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "CreateCompositePackageCatalogOptions.g.h"

const CLSID CLSID_CreateCompositePackageCatalogOptions2 = { 0x6444B10D, 0xFE84, 0x430F, { 0x93, 0x2B, 0x3D, 0x4F, 0xE5, 0x19, 0x5B, 0xDF } }; //6444B10D-FE84-430F-932B-3D4FE5195BDF

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct CreateCompositePackageCatalogOptions : CreateCompositePackageCatalogOptionsT<CreateCompositePackageCatalogOptions>
    {
        CreateCompositePackageCatalogOptions() = default;

        winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageCatalogReference> Catalogs();
        winrt::Microsoft::Management::Deployment::CompositeSearchBehavior CompositeSearchBehavior();
        void CompositeSearchBehavior(winrt::Microsoft::Management::Deployment::CompositeSearchBehavior const& value);
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct CreateCompositePackageCatalogOptions : CreateCompositePackageCatalogOptionsT<CreateCompositePackageCatalogOptions, implementation::CreateCompositePackageCatalogOptions>
    {
        auto ActivateInstance() const
        {
            return winrt::create_instance<winrt::Microsoft::Management::Deployment::PackageManager>(CLSID_CreateCompositePackageCatalogOptions2, CLSCTX_ALL);
        }
    };
}
