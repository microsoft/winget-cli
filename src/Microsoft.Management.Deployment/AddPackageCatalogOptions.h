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

        bool AcceptSourceAgreements();
        void AcceptSourceAgreements(bool const& value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        std::wstring m_name = L"";
        std::wstring m_sourceUri = L"";
        std::wstring m_type = L"";
        winrt::Microsoft::Management::Deployment::PackageCatalogTrustLevel m_trustLevel = winrt::Microsoft::Management::Deployment::PackageCatalogTrustLevel::None;
        std::wstring m_customHeader = L"";
        bool m_explicit = false;
        bool m_acceptSourceAgreements = false;
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
