// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "AddPackageCatalogOptions.g.h"
#include "public/ComClsids.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid(WINGET_OUTOFPROC_COM_CLSID_AddPackageCatalogOptions)]
        struct AddPackageCatalogOptions : AddPackageCatalogOptionsT<AddPackageCatalogOptions>
    {
        AddPackageCatalogOptions() = default;

        hstring CatalogName();
        void CatalogName(hstring const& value);

        hstring SourceUri();
        void SourceUri(hstring const& value);

        hstring Type();
        void Type(hstring const& value);

        hstring Arguments();
        void Arguments(hstring const& value);

        winrt::Microsoft::Management::Deployment::PackageCatalogTrustLevel TrustLevel();
        void TrustLevel(winrt::Microsoft::Management::Deployment::PackageCatalogTrustLevel const& value);

        hstring CustomHeader();
        void CustomHeader(hstring const& value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        std::wstring m_catalogName = L"";
        std::wstring m_sourceUri = L"";
        std::wstring m_type = L"";
        std::wstring m_arguments = L"";
        winrt::Microsoft::Management::Deployment::PackageCatalogTrustLevel m_trustLevel = winrt::Microsoft::Management::Deployment::PackageCatalogTrustLevel::None;
        std::wstring m_customHeader = L"";
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct AddPackageCatalogOptions : AddPackageCatalogOptionsT<AddPackageCatalogOptions, implementation::AddPackageCatalogOptions>
    {
    };
}
#endif
