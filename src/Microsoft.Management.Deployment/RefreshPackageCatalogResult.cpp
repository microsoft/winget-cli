// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RefreshPackageCatalogResult.h"
#include "RefreshPackageCatalogResult.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void RefreshPackageCatalogResult::Initialize(
        winrt::Microsoft::Management::Deployment::RefreshPackageCatalogStatus status,
        winrt::hresult extendedErrorCode)
    {
        m_status = status;
        m_extendedErrorCode = extendedErrorCode;
    }

    winrt::Microsoft::Management::Deployment::RefreshPackageCatalogStatus RefreshPackageCatalogResult::Status()
    {
        return m_status;
    }

    winrt::hresult RefreshPackageCatalogResult::ExtendedErrorCode()
    {
        return m_extendedErrorCode;
    }
}
