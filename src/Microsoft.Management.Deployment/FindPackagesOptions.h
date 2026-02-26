// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "FindPackagesOptions.g.h"
#include "Public/ComClsids.h"
#include <winget/ModuleCountBase.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid(WINGET_OUTOFPROC_COM_CLSID_FindPackagesOptions)]
    struct FindPackagesOptions : FindPackagesOptionsT<FindPackagesOptions>
    {
        FindPackagesOptions() = default;

        winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageMatchFilter> Selectors();
        winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageMatchFilter> Filters();
        uint32_t ResultLimit();
        void ResultLimit(uint32_t value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        uint32_t m_resultLimit = 0;
        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::PackageMatchFilter> m_selectors{ 
            winrt::single_threaded_vector<winrt::Microsoft::Management::Deployment::PackageMatchFilter>() };
        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::PackageMatchFilter> m_filters{ 
            winrt::single_threaded_vector<winrt::Microsoft::Management::Deployment::PackageMatchFilter>() };
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct FindPackagesOptions : FindPackagesOptionsT<FindPackagesOptions, implementation::FindPackagesOptions>, AppInstaller::WinRT::ModuleCountBase
    {
    };
}
#endif
