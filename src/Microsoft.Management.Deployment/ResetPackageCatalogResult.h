// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ResetPackageCatalogResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct ResetPackageCatalogResult : ResetPackageCatalogResultT<ResetPackageCatalogResult>
    {
        ResetPackageCatalogResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(
            winrt::Microsoft::Management::Deployment::ResetPackageCatalogStatus status,
            winrt::hresult extendedErrorCode);
#endif

        winrt::Microsoft::Management::Deployment::ResetPackageCatalogStatus Status();
        winrt::hresult ExtendedErrorCode();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::ResetPackageCatalogStatus m_status = winrt::Microsoft::Management::Deployment::ResetPackageCatalogStatus::Ok;
        winrt::hresult m_extendedErrorCode = S_OK;
#endif
    };
}
