// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageInstallerInfo.g.h"
#include <winget/ManifestInstaller.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageInstallerInfo : PackageInstallerInfoT<PackageInstallerInfo>
    {
        PackageInstallerInfo() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(const ::AppInstaller::Manifest::ManifestInstaller& manifestInstaller);
#endif

        winrt::Microsoft::Management::Deployment::PackageInstallerType InstallerType();
        winrt::Microsoft::Management::Deployment::PackageInstallerType NestedInstallerType();
        winrt::Windows::System::ProcessorArchitecture Architecture();
        winrt::Microsoft::Management::Deployment::PackageInstallerScope Scope();
        hstring Locale();
        // Contract 6.0
        winrt::Microsoft::Management::Deployment::ElevationRequirement ElevationRequirement();
        
#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ::AppInstaller::Manifest::ManifestInstaller m_manifestInstaller;
#endif
    };
}
