// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConnectResult.h"
#include "ConnectResult.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void ConnectResult::Initialize(winrt::Microsoft::Management::Deployment::ConnectResultStatus status, winrt::Microsoft::Management::Deployment::PackageCatalog packageCatalog)
    {
        m_status = status;
        m_packageCatalog = packageCatalog;
    }
    winrt::Microsoft::Management::Deployment::ConnectResultStatus ConnectResult::Status()
    {
        return m_status;
    }
    winrt::Microsoft::Management::Deployment::PackageCatalog ConnectResult::PackageCatalog()
    {
        return m_packageCatalog;
    }
}
