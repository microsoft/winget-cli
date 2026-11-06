// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PinPackageResult.h"
#include "PinPackageResult.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void PinPackageResult::Initialize(
        winrt::Microsoft::Management::Deployment::PinResultStatus status,
        winrt::hresult extendedErrorCode)
    {
        m_status = status;
        m_extendedErrorCode = extendedErrorCode;
    }

    winrt::Microsoft::Management::Deployment::PinResultStatus PinPackageResult::Status()
    {
        return m_status;
    }

    winrt::hresult PinPackageResult::ExtendedErrorCode()
    {
        return m_extendedErrorCode;
    }
}
