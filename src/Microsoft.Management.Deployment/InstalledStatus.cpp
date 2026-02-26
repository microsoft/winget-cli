// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstalledStatus.h"
#include "InstalledStatus.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void InstalledStatus::Initialize(const ::AppInstaller::Repository::InstalledStatus& installedStatus)
    {
        m_type = static_cast<Microsoft::Management::Deployment::InstalledStatusType>(installedStatus.Type);
        m_path = winrt::to_hstring(installedStatus.Path);
        m_status = installedStatus.Status;
    }
    winrt::Microsoft::Management::Deployment::InstalledStatusType InstalledStatus::Type()
    {
        return m_type;
    }
    hstring InstalledStatus::Path()
    {
        return m_path;
    }
    winrt::hresult InstalledStatus::Status()
    {
        return m_status;
    }
}
