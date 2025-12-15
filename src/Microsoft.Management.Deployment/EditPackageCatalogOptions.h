// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "EditPackageCatalogOptions.g.h"
#include "public/ComClsids.h"
#include <winget/ModuleCountBase.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid(WINGET_OUTOFPROC_COM_CLSID_EditPackageCatalogOptions)]
    struct EditPackageCatalogOptions : EditPackageCatalogOptionsT<EditPackageCatalogOptions>
    {
        EditPackageCatalogOptions() = default;

        hstring Name();
        void Name(hstring const& value);

        OptionalBoolean Explicit();
        void Explicit(OptionalBoolean const& value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        hstring m_name = L"";
        OptionalBoolean m_explicit = OptionalBoolean::Unspecified;
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct EditPackageCatalogOptions :
        EditPackageCatalogOptionsT<EditPackageCatalogOptions, implementation::EditPackageCatalogOptions>,
        AppInstaller::WinRT::ModuleCountBase
    {
    };
}
#endif
