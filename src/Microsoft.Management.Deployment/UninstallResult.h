// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "UninstallResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct UninstallResult : UninstallResultT<UninstallResult>
    {
        UninstallResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(
            winrt::Microsoft::Management::Deployment::UninstallResultStatus status,
            winrt::hresult extendedErrorCode,
            uint32_t uninstallerErrorCode,
            hstring const& correlationData, 
            bool rebootRequired);
#endif

        hstring CorrelationData();
        bool RebootRequired();
        winrt::Microsoft::Management::Deployment::UninstallResultStatus Status();
        winrt::hresult ExtendedErrorCode();
        uint32_t UninstallerErrorCode();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        std::wstring m_correlationData = L"";
        bool m_rebootRequired = false;
        winrt::Microsoft::Management::Deployment::UninstallResultStatus m_status = winrt::Microsoft::Management::Deployment::UninstallResultStatus::Ok;
        winrt::hresult m_extendedErrorCode = S_OK;
        uint32_t m_uninstallerErrorCode = 0;
#endif
    };
}
