// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 6388 Allow CreateInstance.
#include <wil\cppwinrt_wrl.h>
// 4467 Allow use of uuid attribute for com object creation.
#include "UninstallOptions.h"
#pragma warning( pop )
#include "UninstallOptions.g.cpp"
#include "Converters.h"
#include "Helpers.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    UninstallOptions::UninstallOptions()
    {
    }
    winrt::Microsoft::Management::Deployment::PackageVersionId UninstallOptions::PackageVersionId()
    {
        return m_packageVersionId;
    }
    void UninstallOptions::PackageVersionId(winrt::Microsoft::Management::Deployment::PackageVersionId const& value)
    {
        m_packageVersionId = value;
    }
    winrt::Microsoft::Management::Deployment::PackageUninstallMode UninstallOptions::PackageUninstallMode()
    {
        return m_packageUninstallMode;
    }
    void UninstallOptions::PackageUninstallMode(winrt::Microsoft::Management::Deployment::PackageUninstallMode const& value)
    {
        m_packageUninstallMode = value;
    }
    hstring UninstallOptions::LogOutputPath()
    {
        return hstring(m_logOutputPath);
    }
    void UninstallOptions::LogOutputPath(hstring const& value)
    {
        m_logOutputPath = value;
    }
    hstring UninstallOptions::CorrelationData()
    {
        return hstring(m_correlationData);
    }
    void UninstallOptions::CorrelationData(hstring const& value)
    {
        m_correlationData = value;
    }
    bool UninstallOptions::Force()
    {
        return m_force;
    }
    void UninstallOptions::Force(bool value)
    {
        m_force = value;
    }
    winrt::Microsoft::Management::Deployment::PackageUninstallScope UninstallOptions::PackageUninstallScope()
    {
        return m_packageUninstallScope;
    }
    void UninstallOptions::PackageUninstallScope(winrt::Microsoft::Management::Deployment::PackageUninstallScope const& value)
    {
        m_packageUninstallScope = value;
    }

    CoCreatableMicrosoftManagementDeploymentClass(UninstallOptions);
}
