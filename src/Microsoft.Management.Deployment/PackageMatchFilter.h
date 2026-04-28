// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageMatchFilter.g.h"
#include "Public/ComClsids.h"
#include <winget/ModuleCountBase.h>

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace AppInstaller::Repository
{
    struct PackageMatchFilter;
}
#endif

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid(WINGET_OUTOFPROC_COM_CLSID_PackageMatchFilter)]
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
    struct PackageMatchFilter : PackageMatchFilterT<PackageMatchFilter, implementation::PackageMatchFilter>, AppInstaller::WinRT::ModuleCountBase
    {
    };
}
#endif
