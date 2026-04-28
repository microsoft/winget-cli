// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "EditPackageCatalogResult.h"
#include "EditPackageCatalogResult.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void EditPackageCatalogResult::Initialize(
        winrt::Microsoft::Management::Deployment::EditPackageCatalogStatus status,
        winrt::hresult extendedErrorCode)
    {
        m_status = status;
        m_extendedErrorCode = extendedErrorCode;
    }
    winrt::Microsoft::Management::Deployment::EditPackageCatalogStatus EditPackageCatalogResult::Status()
    {
        return m_status;
    }
    winrt::hresult EditPackageCatalogResult::ExtendedErrorCode()
    {
        return m_extendedErrorCode;
    }
}
