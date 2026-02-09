// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "EditPackageCatalogOptions.g.h"
#include "public/ComClsids.h"
#include <winget/ModuleCountBase.h>
#include <winrt/Windows.Foundation.h>
#include <optional>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid(WINGET_OUTOFPROC_COM_CLSID_EditPackageCatalogOptions)]
    struct EditPackageCatalogOptions : EditPackageCatalogOptionsT<EditPackageCatalogOptions>
    {
        EditPackageCatalogOptions() = default;

        hstring Name();
        void Name(hstring const& value);

        Windows::Foundation::IReference<bool> Explicit();
        void Explicit(Windows::Foundation::IReference<bool> value);

        Windows::Foundation::IReference<int32_t> Priority();
        void Priority(Windows::Foundation::IReference<int32_t> value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        hstring m_name = L"";
        std::optional<bool> m_explicit;
        std::optional<int32_t> m_priority;
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
