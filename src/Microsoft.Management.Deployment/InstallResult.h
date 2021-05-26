// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "InstallResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct InstallResult : InstallResultT<InstallResult>
    {
        InstallResult() = default;
        void Initialize(
            winrt::Microsoft::Management::Deployment::InstallResultStatus status,
            winrt::hresult extendedErrorCode, 
            hstring const& correlationData, 
            bool rebootRequired);

        hstring CorrelationData();
        bool RebootRequired();
        winrt::Microsoft::Management::Deployment::InstallResultStatus Status();
        winrt::hresult ExtendedErrorCode();
    private:
        std::wstring m_correlationData = L"";
        bool m_rebootRequired = false;
        winrt::Microsoft::Management::Deployment::InstallResultStatus m_status = winrt::Microsoft::Management::Deployment::InstallResultStatus::Ok;
        winrt::hresult m_extendedErrorCode = S_OK;
    };
}
