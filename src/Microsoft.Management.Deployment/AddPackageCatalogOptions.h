// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "AddPackageCatalogOptions.g.h"
#include "public/ComClsids.h"
#include <winget/ModuleCountBase.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid(WINGET_OUTOFPROC_COM_CLSID_AddPackageCatalogOptions)]
    struct AddPackageCatalogOptions : AddPackageCatalogOptionsT<AddPackageCatalogOptions>
    {
        AddPackageCatalogOptions() = default;

        hstring Name();
        void Name(hstring const& value);

        hstring SourceUri();
        void SourceUri(hstring const& value);

        hstring Type();
        void Type(hstring const& value);

        winrt::Microsoft::Management::Deployment::PackageCatalogTrustLevel TrustLevel();
        void TrustLevel(winrt::Microsoft::Management::Deployment::PackageCatalogTrustLevel const& value);

        hstring CustomHeader();
        void CustomHeader(hstring const& value);

        bool Explicit();
        void Explicit(bool const& value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        hstring m_name = L"";
        hstring m_sourceUri = L"";
        hstring m_type = L"";
        winrt::Microsoft::Management::Deployment::PackageCatalogTrustLevel m_trustLevel = winrt::Microsoft::Management::Deployment::PackageCatalogTrustLevel::None;
        hstring m_customHeader = L"";
        bool m_explicit = false;
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct AddPackageCatalogOptions : AddPackageCatalogOptionsT<AddPackageCatalogOptions, implementation::AddPackageCatalogOptions>, AppInstaller::WinRT::ModuleCountBase
    {
    };
}
#endif
