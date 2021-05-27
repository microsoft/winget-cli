// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "FindPackagesOptions.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct FindPackagesOptions : FindPackagesOptionsT<FindPackagesOptions>
    {
        FindPackagesOptions() = default;

        winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageMatchFilter> Selectors();
        winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageMatchFilter> Filters();
        uint32_t ResultLimit();
        void ResultLimit(uint32_t value);
    private:
        uint32_t m_resultLimit = 0;
        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::PackageMatchFilter> m_selectors{ 
            winrt::single_threaded_vector<winrt::Microsoft::Management::Deployment::PackageMatchFilter>() };
        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::PackageMatchFilter> m_filters{ 
            winrt::single_threaded_vector<winrt::Microsoft::Management::Deployment::PackageMatchFilter>() };
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct FindPackagesOptions : FindPackagesOptionsT<FindPackagesOptions, implementation::FindPackagesOptions>
    {
    };
}
