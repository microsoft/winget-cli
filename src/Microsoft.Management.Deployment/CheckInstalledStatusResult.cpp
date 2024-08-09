// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CheckInstalledStatusResult.h"
#include "CheckInstalledStatusResult.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void CheckInstalledStatusResult::Initialize(
        winrt::Microsoft::Management::Deployment::CheckInstalledStatusResultStatus status,
        Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageInstallerInstalledStatus> installedStatus)
    {
        m_status = status;
        m_installedStatus = installedStatus;
    }
    winrt::Microsoft::Management::Deployment::CheckInstalledStatusResultStatus CheckInstalledStatusResult::Status()
    {
        return m_status;
    }
    winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::PackageInstallerInstalledStatus> CheckInstalledStatusResult::PackageInstalledStatus()
    {
        return m_installedStatus.GetView();
    }
}
