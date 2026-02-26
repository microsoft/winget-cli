// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AddPackageCatalogResult.h"
#include "AddPackageCatalogResult.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void AddPackageCatalogResult::Initialize(
        winrt::Microsoft::Management::Deployment::AddPackageCatalogStatus status,
        winrt::hresult extendedErrorCode)
    {
        m_status = status;
        m_extendedErrorCode = extendedErrorCode;
    }
    winrt::Microsoft::Management::Deployment::AddPackageCatalogStatus AddPackageCatalogResult::Status()
    {
        return m_status;
    }
    winrt::hresult AddPackageCatalogResult::ExtendedErrorCode()
    {
        return m_extendedErrorCode;
    }
}
