// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PinPackageResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PinPackageResult : PinPackageResultT<PinPackageResult>
    {
        PinPackageResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(
            winrt::Microsoft::Management::Deployment::PinResultStatus status,
            winrt::hresult extendedErrorCode);
#endif

        winrt::Microsoft::Management::Deployment::PinResultStatus Status();
        winrt::hresult ExtendedErrorCode();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::PinResultStatus m_status = winrt::Microsoft::Management::Deployment::PinResultStatus::Ok;
        winrt::hresult m_extendedErrorCode = S_OK;
#endif
    };
}
