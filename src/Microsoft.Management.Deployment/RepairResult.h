// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "RepairResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct RepairResult : RepairResultT<RepairResult>
    {
        RepairResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(
            winrt::Microsoft::Management::Deployment::RepairResultStatus status,
            winrt::hresult extendedErrorCode,
            uint32_t repairerErrorCode,
            hstring const& correlationData,
            bool rebootRequired);
#endif

        hstring CorrelationData();
        bool RebootRequired();
        winrt::Microsoft::Management::Deployment::RepairResultStatus Status();
        winrt::hresult ExtendedErrorCode();
        uint32_t RepairerErrorCode();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        std::wstring m_correlationData = L"";
        bool m_rebootRequired = false;
        winrt::Microsoft::Management::Deployment::RepairResultStatus m_status = winrt::Microsoft::Management::Deployment::RepairResultStatus::Ok;
        winrt::hresult m_extendedErrorCode = S_OK;
        uint32_t m_repairerErrorCode = 0;
#endif
    };
}
