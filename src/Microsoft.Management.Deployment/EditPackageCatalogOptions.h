// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "EditPackageCatalogOptions.g.h"
#include "public/ComClsids.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid(WINGET_OUTOFPROC_COM_CLSID_EditPackageCatalogOptions)]
    struct EditPackageCatalogOptions : EditPackageCatalogOptionsT<EditPackageCatalogOptions>
    {
        EditPackageCatalogOptions() = default;

        hstring Name();
        void Name(hstring const& value);

        hstring Explicit();
        void Explicit(hstring const& value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        hstring m_name = L"";
        hstring m_explicit = L"";
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct EditPackageCatalogOptions : EditPackageCatalogOptionsT<EditPackageCatalogOptions, implementation::EditPackageCatalogOptions>
    {
    };
}
#endif
