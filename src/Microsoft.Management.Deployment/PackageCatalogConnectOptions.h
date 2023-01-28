// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageCatalogConnectOptions.g.h"
#include "Public/ComClsids.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid(WINGET_OUTOFPROC_COM_CLSID_PackageCatalogConnectOptions)]
    struct PackageCatalogConnectOptions : PackageCatalogConnectOptionsT<PackageCatalogConnectOptions>
    {
        PackageCatalogConnectOptions() = default;

        bool AcceptSourceAgreements();
        void AcceptSourceAgreements(bool value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        bool m_acceptSourceAgreements = true;
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct PackageCatalogConnectOptions : PackageCatalogConnectOptionsT<PackageCatalogConnectOptions, implementation::PackageCatalogConnectOptions>
    {
    };
}
#endif
