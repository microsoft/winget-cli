// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "EditPackageCatalogResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct EditPackageCatalogResult : EditPackageCatalogResultT<EditPackageCatalogResult>
    {
        EditPackageCatalogResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(
            winrt::Microsoft::Management::Deployment::EditPackageCatalogStatus status,
            winrt::hresult extendedErrorCode);
#endif

        winrt::Microsoft::Management::Deployment::EditPackageCatalogStatus Status();
        winrt::hresult ExtendedErrorCode();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::EditPackageCatalogStatus m_status = winrt::Microsoft::Management::Deployment::EditPackageCatalogStatus::Ok;
        winrt::hresult m_extendedErrorCode = S_OK;
#endif
    };
}
