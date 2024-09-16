// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ResetPackageCatalogResult.h"
#include "ResetPackageCatalogResult.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void ResetPackageCatalogResult::Initialize(
        winrt::Microsoft::Management::Deployment::ResetPackageCatalogStatus status,
        winrt::hresult extendedErrorCode)
    {
        m_status = status;
        m_extendedErrorCode = extendedErrorCode;
    }
    winrt::Microsoft::Management::Deployment::ResetPackageCatalogStatus ResetPackageCatalogResult::Status()
    {
        return m_status;
    }
    winrt::hresult ResetPackageCatalogResult::ExtendedErrorCode()
    {
        return m_extendedErrorCode;
    }
}
