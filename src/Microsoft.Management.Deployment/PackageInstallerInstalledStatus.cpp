// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PackageInstallerInstalledStatus.h"
#include "PackageInstallerInstalledStatus.g.cpp"
#include "InstalledStatus.h"
#include "PackageInstallerInfo.h"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void PackageInstallerInstalledStatus::Initialize(const ::AppInstaller::Repository::InstallerInstalledStatus& installerInstalledStatus)
    {
        // Initialize m_installerInfo
        auto installerInfo = winrt::make_self<wil::details::module_count_wrapper<
            winrt::Microsoft::Management::Deployment::implementation::PackageInstallerInfo>>();
        installerInfo->Initialize(installerInstalledStatus.Installer);
        m_installerInfo = *installerInfo;

        // Initialize m_installedStatus
        for (auto const& entry : installerInstalledStatus.Status)
        {
            auto status = winrt::make_self<wil::details::module_count_wrapper<
                winrt::Microsoft::Management::Deployment::implementation::InstalledStatus>>();
            status->Initialize(entry);

            m_installedStatus.Append(*status);
        }
    }
    winrt::Microsoft::Management::Deployment::PackageInstallerInfo PackageInstallerInstalledStatus::InstallerInfo()
    {
        return m_installerInfo;
    }
    winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::InstalledStatus> PackageInstallerInstalledStatus::InstallerInstalledStatus()
    {
        return m_installedStatus.GetView();
    }
}
