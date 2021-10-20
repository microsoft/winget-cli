// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageMatchFilter.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
#if USE_PROD_CLSIDS 
    [uuid("D02C9DAF-99DC-429C-B503-4E504E4AB000")]
#else
    [uuid("3F85B9F4-487A-4C48-9035-2903F8A6D9E8")]
#endif
    struct PackageMatchFilter : PackageMatchFilterT<PackageMatchFilter>
    {
        PackageMatchFilter() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(::AppInstaller::Repository::PackageMatchFilter matchFilter);
#endif

        winrt::Microsoft::Management::Deployment::PackageFieldMatchOption Option();
        void Option(winrt::Microsoft::Management::Deployment::PackageFieldMatchOption const& value);
        winrt::Microsoft::Management::Deployment::PackageMatchField Field();
        void Field(winrt::Microsoft::Management::Deployment::PackageMatchField const& value);
        hstring Value();
        void Value(hstring const& value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        hstring m_value;
        winrt::Microsoft::Management::Deployment::PackageMatchField m_matchField = winrt::Microsoft::Management::Deployment::PackageMatchField::CatalogDefault;
        winrt::Microsoft::Management::Deployment::PackageFieldMatchOption m_packageFieldMatchOption = winrt::Microsoft::Management::Deployment::PackageFieldMatchOption::Equals;
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct PackageMatchFilter : PackageMatchFilterT<PackageMatchFilter, implementation::PackageMatchFilter>
    {
    };
}
#endif
