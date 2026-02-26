// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RemovePackageCatalogResult.h"
#include "RemovePackageCatalogResult.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void RemovePackageCatalogResult::Initialize(
        winrt::Microsoft::Management::Deployment::RemovePackageCatalogStatus status,
        winrt::hresult extendedErrorCode)
    {
        m_status = status;
        m_extendedErrorCode = extendedErrorCode;
    }
    winrt::Microsoft::Management::Deployment::RemovePackageCatalogStatus RemovePackageCatalogResult::Status()
    {
        return m_status;
    }
    winrt::hresult RemovePackageCatalogResult::ExtendedErrorCode()
    {
        return m_extendedErrorCode;
    }
}
