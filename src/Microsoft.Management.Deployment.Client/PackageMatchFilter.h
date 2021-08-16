// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageMatchFilter.g.h"

const CLSID CLSID_PackageMatchFilter = { 0xADBF3B4A, 0xDB8A, 0x496C, { 0xA5, 0x79, 0x62, 0xB5, 0x8F, 0x5F, 0xB1, 0x30 } }; //ADBF3B4A-DB8A-496C-A579-62B58F5FB13F

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageMatchFilter : PackageMatchFilterT<PackageMatchFilter>
    {
        PackageMatchFilter() = default;

        winrt::Microsoft::Management::Deployment::PackageFieldMatchOption Option();
        void Option(winrt::Microsoft::Management::Deployment::PackageFieldMatchOption const& value);
        winrt::Microsoft::Management::Deployment::PackageMatchField Field();
        void Field(winrt::Microsoft::Management::Deployment::PackageMatchField const& value);
        hstring Value();
        void Value(hstring const& value);
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct PackageMatchFilter : PackageMatchFilterT<PackageMatchFilter, implementation::PackageMatchFilter>
    {
        auto ActivateInstance() const
        {
            return winrt::create_instance<winrt::Microsoft::Management::Deployment::PackageManager>(CLSID_PackageMatchFilter, CLSCTX_ALL);
        }
    };
}
