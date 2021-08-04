// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallOptions.h"
#include "InstallOptions.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Microsoft::Management::Deployment::PackageVersionId InstallOptions::PackageVersionId()
    {
        return m_installOptions.PackageVersionId();
    }
    void InstallOptions::PackageVersionId(winrt::Microsoft::Management::Deployment::PackageVersionId const& value)
    {
        return m_installOptions.PackageVersionId(value);
    }
    hstring InstallOptions::PreferredInstallLocation()
    {
        return m_installOptions.PreferredInstallLocation();
    }
    void InstallOptions::PreferredInstallLocation(hstring const& value)
    {
        return m_installOptions.PreferredInstallLocation(value);
    }
    winrt::Microsoft::Management::Deployment::PackageInstallScope InstallOptions::PackageInstallScope()
    {
        return m_installOptions.PackageInstallScope();
    }
    void InstallOptions::PackageInstallScope(winrt::Microsoft::Management::Deployment::PackageInstallScope const& value)
    {
        return m_installOptions.PackageInstallScope(value);
    }
    winrt::Microsoft::Management::Deployment::PackageInstallMode InstallOptions::PackageInstallMode()
    {
        return m_installOptions.PackageInstallMode();
    }
    void InstallOptions::PackageInstallMode(winrt::Microsoft::Management::Deployment::PackageInstallMode const& value)
    {
        return m_installOptions.PackageInstallMode(value);
    }
    hstring InstallOptions::LogOutputPath()
    {
        return m_installOptions.LogOutputPath();
    }
    void InstallOptions::LogOutputPath(hstring const& value)
    {
        return m_installOptions.LogOutputPath(value);
    }
    bool InstallOptions::AllowHashMismatch()
    {
        return m_installOptions.AllowHashMismatch();
    }
    void InstallOptions::AllowHashMismatch(bool value)
    {
        return m_installOptions.AllowHashMismatch(value);
    }
    hstring InstallOptions::ReplacementInstallerArguments()
    {
        return m_installOptions.ReplacementInstallerArguments();
    }
    void InstallOptions::ReplacementInstallerArguments(hstring const& value)
    {
        return m_installOptions.ReplacementInstallerArguments(value);
    }
    hstring InstallOptions::CorrelationData()
    {
        return m_installOptions.CorrelationData();
    }
    void InstallOptions::CorrelationData(hstring const& value)
    {
        return m_installOptions.CorrelationData(value);
    }
    hstring InstallOptions::AdditionalPackageCatalogArguments()
    {
        return m_installOptions.AdditionalPackageCatalogArguments();
    }
    void InstallOptions::AdditionalPackageCatalogArguments(hstring const& value)
    {
        return m_installOptions.AdditionalPackageCatalogArguments(value);
    }
}
