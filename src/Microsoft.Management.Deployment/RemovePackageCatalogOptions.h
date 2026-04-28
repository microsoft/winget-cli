// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "RemovePackageCatalogOptions.g.h"
#include "public/ComClsids.h"
#include <winget/ModuleCountBase.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid(WINGET_OUTOFPROC_COM_CLSID_RemovePackageCatalogOptions)]
    struct RemovePackageCatalogOptions : RemovePackageCatalogOptionsT<RemovePackageCatalogOptions>
    {
        RemovePackageCatalogOptions() = default;

        hstring Name();
        void Name(hstring const& value);

        bool PreserveData();
        void PreserveData(bool const& value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        hstring m_name = L"";
        bool m_preserveData = false;
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct RemovePackageCatalogOptions :
        RemovePackageCatalogOptionsT<RemovePackageCatalogOptions, implementation::RemovePackageCatalogOptions>,
        AppInstaller::WinRT::ModuleCountBase
    {
    };
}
#endif
