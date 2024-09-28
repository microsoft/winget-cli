// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "RepairResult.h"
#include "RepairResult.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void RepairResult::Initialize(
        winrt::Microsoft::Management::Deployment::RepairResultStatus status,
        winrt::hresult extendedErrorCode,
        uint32_t repairerErrorCode,
        hstring const& correlationData,
        bool rebootRequired)
    {
        m_status = status;
        m_extendedErrorCode = extendedErrorCode;
        m_repairerErrorCode = repairerErrorCode;
        m_correlationData = correlationData;
        m_rebootRequired = rebootRequired;
    }
    hstring RepairResult::CorrelationData()
    {
        return hstring(m_correlationData);
    }
    bool RepairResult::RebootRequired()
    {
        return m_rebootRequired;
    }
    winrt::Microsoft::Management::Deployment::RepairResultStatus RepairResult::Status()
    {
        return m_status;
    }
    winrt::hresult RepairResult::ExtendedErrorCode()
    {
        return m_extendedErrorCode;
    }
    uint32_t RepairResult::RepairerErrorCode()
    {
        return m_repairerErrorCode;
    }
}
