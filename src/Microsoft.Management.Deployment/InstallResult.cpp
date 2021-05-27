// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallResult.h"
#include "InstallResult.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void InstallResult::Initialize(
        winrt::Microsoft::Management::Deployment::InstallResultStatus status,
        winrt::hresult extendedErrorCode, 
        hstring const& correlationData, 
        bool rebootRequired)
    {
        m_status = status;
        m_extendedErrorCode = extendedErrorCode;
        m_correlationData = correlationData;
        m_rebootRequired = rebootRequired;
    }
    hstring InstallResult::CorrelationData()
    {
        return hstring(m_correlationData);
    }
    bool InstallResult::RebootRequired()
    {
        return m_rebootRequired;
    }
    winrt::Microsoft::Management::Deployment::InstallResultStatus InstallResult::Status()
    {
        return m_status;
    }
    winrt::hresult InstallResult::ExtendedErrorCode()
    {
        return m_extendedErrorCode;
    }
}
