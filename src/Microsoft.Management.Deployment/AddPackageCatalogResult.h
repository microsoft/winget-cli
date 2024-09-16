// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "AddPackageCatalogResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct AddPackageCatalogResult : AddPackageCatalogResultT<AddPackageCatalogResult>
    {
        AddPackageCatalogResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(
            winrt::Microsoft::Management::Deployment::AddPackageCatalogStatus status,
            winrt::hresult extendedErrorCode);
#endif

        winrt::Microsoft::Management::Deployment::AddPackageCatalogStatus Status();
        winrt::hresult ExtendedErrorCode();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::AddPackageCatalogStatus m_status = winrt::Microsoft::Management::Deployment::AddPackageCatalogStatus::Ok;
        winrt::hresult m_extendedErrorCode = S_OK;
#endif
    };
}
#pragma once
