// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "UninstallResult.h"
#include "UninstallResult.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void UninstallResult::Initialize(
        winrt::Microsoft::Management::Deployment::UninstallResultStatus status,
        winrt::hresult extendedErrorCode,
        uint32_t uninstallerErrorCode,
        hstring const& correlationData, 
        bool rebootRequired)
    {
        m_status = status;
        m_extendedErrorCode = extendedErrorCode;
        m_uninstallerErrorCode = uninstallerErrorCode;
        m_correlationData = correlationData;
        m_rebootRequired = rebootRequired;
    }
    hstring UninstallResult::CorrelationData()
    {
        return hstring(m_correlationData);
    }
    bool UninstallResult::RebootRequired()
    {
        return m_rebootRequired;
    }
    winrt::Microsoft::Management::Deployment::UninstallResultStatus UninstallResult::Status()
    {
        return m_status;
    }
    winrt::hresult UninstallResult::ExtendedErrorCode()
    {
        return m_extendedErrorCode;
    }

    uint32_t UninstallResult::UninstallerErrorCode()
    {
        return m_uninstallerErrorCode;
    }
}
