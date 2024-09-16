// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "UpdatePackageCatalogResult.h"
#include "UpdatePackageCatalogResult.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void UpdatePackageCatalogResult::Initialize(
        winrt::Microsoft::Management::Deployment::UpdatePackageCatalogStatus status,
        winrt::hresult extendedErrorCode)
    {
        m_status = status;
        m_extendedErrorCode = extendedErrorCode;
    }

    winrt::Microsoft::Management::Deployment::UpdatePackageCatalogStatus UpdatePackageCatalogResult::Status()
    {
        return m_status;
    }

    winrt::hresult UpdatePackageCatalogResult::ExtendedErrorCode()
    {
        return m_extendedErrorCode;
    }
}
