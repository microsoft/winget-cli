// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "FindPackagesOptions.g.h"

const CLSID CLSID_FindPackagesOptions2 = { 0x2CAD6C15, 0xDF8E, 0x49DD, { 0xA7, 0x48, 0x96, 0xAD, 0xE0, 0xFE, 0x31, 0xB7 } }; //2CAD6C15-DF8E-49DD-A748-96ADE0FE31B7

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct FindPackagesOptions : FindPackagesOptionsT<FindPackagesOptions>
    {
        FindPackagesOptions() = default;

        winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageMatchFilter> Selectors();
        winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageMatchFilter> Filters();
        uint32_t ResultLimit();
        void ResultLimit(uint32_t value);
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct FindPackagesOptions : FindPackagesOptionsT<FindPackagesOptions, implementation::FindPackagesOptions>
    {
        auto ActivateInstance() const
        {
            return winrt::create_instance<winrt::Microsoft::Management::Deployment::PackageManager>(CLSID_FindPackagesOptions2, CLSCTX_ALL);
        }
    };
}
