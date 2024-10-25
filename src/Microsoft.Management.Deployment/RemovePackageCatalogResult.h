// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "RemovePackageCatalogResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct RemovePackageCatalogResult : RemovePackageCatalogResultT<RemovePackageCatalogResult>
    {
        RemovePackageCatalogResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(
            winrt::Microsoft::Management::Deployment::RemovePackageCatalogStatus status,
            winrt::hresult extendedErrorCode);
#endif

        winrt::Microsoft::Management::Deployment::RemovePackageCatalogStatus Status();
        winrt::hresult ExtendedErrorCode();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::RemovePackageCatalogStatus m_status = winrt::Microsoft::Management::Deployment::RemovePackageCatalogStatus::Ok;
        winrt::hresult m_extendedErrorCode = S_OK;
#endif
    };
}
