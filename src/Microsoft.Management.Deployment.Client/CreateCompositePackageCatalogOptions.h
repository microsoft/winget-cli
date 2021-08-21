// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "CreateCompositePackageCatalogOptions.g.h"

const CLSID CLSID_CreateCompositePackageCatalogOptions = { 0xEE160901, 0xB317, 0x4EA7, { 0x9C, 0xC6, 0x53, 0x55, 0xC6, 0xD7, 0xD8, 0xA7 } }; //EE160901-B317-4EA7-9CC6-5355C6D7D8A7

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
            return winrt::create_instance<winrt::Microsoft::Management::Deployment::PackageManager>(CLSID_CreateCompositePackageCatalogOptions, CLSCTX_ALL);
        }
    };
}
