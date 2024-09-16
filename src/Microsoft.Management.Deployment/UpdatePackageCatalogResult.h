// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "UpdatePackageCatalogResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct UpdatePackageCatalogResult : UpdatePackageCatalogResultT<UpdatePackageCatalogResult>
    {
        UpdatePackageCatalogResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(
            winrt::Microsoft::Management::Deployment::UpdatePackageCatalogStatus status,
            winrt::hresult extendedErrorCode);
#endif

        winrt::Microsoft::Management::Deployment::UpdatePackageCatalogStatus Status();
        winrt::hresult ExtendedErrorCode();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::UpdatePackageCatalogStatus m_status = winrt::Microsoft::Management::Deployment::UpdatePackageCatalogStatus::Ok;
        winrt::hresult m_extendedErrorCode = S_OK;
#endif
    };
}
