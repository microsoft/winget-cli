// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageInstallerInstalledStatus.g.h"
#include <winget/RepositorySearch.h>
#include <winget/InstalledStatus.h>
#include <winrt/Windows.Foundation.Collections.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageInstallerInstalledStatus : PackageInstallerInstalledStatusT<PackageInstallerInstalledStatus>
    {
        PackageInstallerInstalledStatus() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(const ::AppInstaller::Repository::InstallerInstalledStatus& installerInstalledStatus);
#endif

        winrt::Microsoft::Management::Deployment::PackageInstallerInfo InstallerInfo();
        winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::InstalledStatus> InstallerInstalledStatus();
        
#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::PackageInstallerInfo m_installerInfo{ nullptr };
        winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::InstalledStatus> m_installedStatus{
            winrt::single_threaded_vector<winrt::Microsoft::Management::Deployment::InstalledStatus>() };
#endif
    };
}
