#include "pch.h"
#include "InstallOptions.h"
#include "InstallOptions.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Microsoft::Management::Deployment::PackageVersionId InstallOptions::PackageVersionId()
    {
        return m_packageVersionId;
    }
    void InstallOptions::PackageVersionId(winrt::Microsoft::Management::Deployment::PackageVersionId const& value)
    {
        m_packageVersionId = value;
    }
    hstring InstallOptions::PreferredInstallLocation()
    {
        return hstring(m_preferredInstallLocation);
    }
    void InstallOptions::PreferredInstallLocation(hstring const& value)
    {
        m_preferredInstallLocation = value;
    }
    winrt::Microsoft::Management::Deployment::AppInstallScope InstallOptions::AppInstallScope()
    {
        return m_appInstallScope;
    }
    void InstallOptions::AppInstallScope(winrt::Microsoft::Management::Deployment::AppInstallScope const& value)
    {
        m_appInstallScope = value;
    }
    winrt::Microsoft::Management::Deployment::AppInstallMode InstallOptions::AppInstallMode()
    {
        return m_appInstallMode;
    }
    void InstallOptions::AppInstallMode(winrt::Microsoft::Management::Deployment::AppInstallMode const& value)
    {
        m_appInstallMode = value;
    }
    hstring InstallOptions::LogOutputPath()
    {
        return hstring(m_logOutputPath);
    }
    void InstallOptions::LogOutputPath(hstring const& value)
    {
        m_logOutputPath = value;
    }
    bool InstallOptions::AllowHashMismatch()
    {
        return m_allowHashMismatch;
    }
    void InstallOptions::AllowHashMismatch(bool value)
    {
        m_allowHashMismatch = value;
    }
    hstring InstallOptions::ReplacementInstallerArguments()
    {
        return hstring(m_replacementInstallerArguments);
    }
    void InstallOptions::ReplacementInstallerArguments(hstring const& value)
    {
        m_replacementInstallerArguments = value;
    }
    hstring InstallOptions::CorrelationData()
    {
        return hstring(m_correlationData);
    }
    void InstallOptions::CorrelationData(hstring const& value)
    {
        m_correlationData = value;
    }
    hstring InstallOptions::AdditionalAppCatalogArguments()
    {
        return hstring(m_additionalAppCatalogArguments);
    }
    void InstallOptions::AdditionalAppCatalogArguments(hstring const& value)
    {
        m_additionalAppCatalogArguments = value;
    }
    CoCreatableCppWinRtClass(InstallOptions);
}
