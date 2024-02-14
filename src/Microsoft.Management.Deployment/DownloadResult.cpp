// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DownloadResult.h"
#include "DownloadResult.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void DownloadResult::Initialize(
        winrt::Microsoft::Management::Deployment::DownloadResultStatus status,
        winrt::hresult extendedErrorCode,
        hstring const& correlationData)
    {
        m_status = status;
        m_extendedErrorCode = extendedErrorCode;
        m_correlationData = correlationData;
    }
    hstring DownloadResult::CorrelationData()
    {
        return hstring(m_correlationData);
    }
    winrt::Microsoft::Management::Deployment::DownloadResultStatus DownloadResult::Status()
    {
        return m_status;
    }
    winrt::hresult DownloadResult::ExtendedErrorCode()
    {
        return m_extendedErrorCode;
    }
}
