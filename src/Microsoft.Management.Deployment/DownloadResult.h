// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "DownloadResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct DownloadResult : DownloadResultT<DownloadResult>
    {
        DownloadResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(
            winrt::Microsoft::Management::Deployment::DownloadResultStatus status,
            winrt::hresult extendedErrorCode,
            hstring const& correlationData);
#endif

        hstring CorrelationData();
        winrt::Microsoft::Management::Deployment::DownloadResultStatus Status();
        winrt::hresult ExtendedErrorCode();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        std::wstring m_correlationData = L"";
        winrt::Microsoft::Management::Deployment::DownloadResultStatus m_status = winrt::Microsoft::Management::Deployment::DownloadResultStatus::Ok;
        winrt::hresult m_extendedErrorCode = S_OK;
#endif
    };
}
