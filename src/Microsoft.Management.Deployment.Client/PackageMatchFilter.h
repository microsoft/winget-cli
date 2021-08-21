// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageMatchFilter.g.h"

const CLSID CLSID_PackageMatchFilter = { 0x3F85B9F4, 0x487A, 0x4C48, { 0x90, 0x35, 0x29, 0x03, 0xF8, 0xA6, 0xD9, 0xE8 } }; //3F85B9F4-487A-4C48-9035-2903F8A6D9E8

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
