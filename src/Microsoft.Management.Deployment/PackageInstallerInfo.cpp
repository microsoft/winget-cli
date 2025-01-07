// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PackageInstallerInfo.h"
#include "PackageInstallerInfo.g.cpp"
#include "AuthenticationInfo.h"
#include "Converters.h"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void PackageInstallerInfo::Initialize(const ::AppInstaller::Manifest::ManifestInstaller& manifestInstaller)
    {
        m_manifestInstaller = manifestInstaller;
    }
    winrt::Microsoft::Management::Deployment::PackageInstallerType PackageInstallerInfo::InstallerType()
    {
        return GetDeploymentInstallerType(m_manifestInstaller.BaseInstallerType);
    }
    winrt::Microsoft::Management::Deployment::PackageInstallerType PackageInstallerInfo::NestedInstallerType()
    {
        return GetDeploymentInstallerType(m_manifestInstaller.NestedInstallerType);
    }
    winrt::Windows::System::ProcessorArchitecture PackageInstallerInfo::Architecture()
    {
        auto convertedArchitecture = GetWindowsSystemProcessorArchitecture(m_manifestInstaller.Arch);
        return convertedArchitecture ? convertedArchitecture.value() : Windows::System::ProcessorArchitecture::Unknown;
    }
    winrt::Microsoft::Management::Deployment::PackageInstallerScope PackageInstallerInfo::Scope()
    {
        return GetDeploymentInstallerScope(m_manifestInstaller.Scope);
    }
    hstring PackageInstallerInfo::Locale()
    {
        return winrt::to_hstring(m_manifestInstaller.Locale);
    }
    winrt::Microsoft::Management::Deployment::ElevationRequirement PackageInstallerInfo::ElevationRequirement()
    {
        return GetDeploymentElevationRequirement(m_manifestInstaller.ElevationRequirement);
    }
    winrt::Microsoft::Management::Deployment::AuthenticationInfo PackageInstallerInfo::AuthenticationInfo()
    {
        std::call_once(m_authenticationInfoOnceFlag,
            [&]()
            {
                auto authenticationInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::AuthenticationInfo>>();
                authenticationInfo->Initialize(m_manifestInstaller.AuthInfo);
                m_authenticationInfo = *authenticationInfo;
            });
        return m_authenticationInfo;
    }
}
