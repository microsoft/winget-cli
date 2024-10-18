// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "RefreshPackageCatalogResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct RefreshPackageCatalogResult : RefreshPackageCatalogResultT<RefreshPackageCatalogResult>
    {
        RefreshPackageCatalogResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(
            winrt::Microsoft::Management::Deployment::RefreshPackageCatalogStatus status,
            winrt::hresult extendedErrorCode);
#endif

        winrt::Microsoft::Management::Deployment::RefreshPackageCatalogStatus Status();
        winrt::hresult ExtendedErrorCode();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::RefreshPackageCatalogStatus m_status = winrt::Microsoft::Management::Deployment::RefreshPackageCatalogStatus::Ok;
        winrt::hresult m_extendedErrorCode = S_OK;
#endif
    };
}
