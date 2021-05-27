// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageMatchFilter.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid("D02C9DAF-99DC-429C-B503-4E504E4AB000")]
    struct PackageMatchFilter : PackageMatchFilterT<PackageMatchFilter>
    {
        PackageMatchFilter() = default;
        void Initialize(::AppInstaller::Repository::PackageMatchFilter matchFilter);

        winrt::Microsoft::Management::Deployment::PackageFieldMatchOption Option();
        void Option(winrt::Microsoft::Management::Deployment::PackageFieldMatchOption const& value);
        winrt::Microsoft::Management::Deployment::PackageMatchField Field();
        void Field(winrt::Microsoft::Management::Deployment::PackageMatchField const& value);
        hstring Value();
        void Value(hstring const& value);
    private:
        hstring m_value;
        winrt::Microsoft::Management::Deployment::PackageMatchField m_matchField = winrt::Microsoft::Management::Deployment::PackageMatchField::CatalogDefault;
        winrt::Microsoft::Management::Deployment::PackageFieldMatchOption m_packageFieldMatchOption = winrt::Microsoft::Management::Deployment::PackageFieldMatchOption::Equals;
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct PackageMatchFilter : PackageMatchFilterT<PackageMatchFilter, implementation::PackageMatchFilter>
    {
    };
}
