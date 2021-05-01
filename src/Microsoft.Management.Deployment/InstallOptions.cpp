#include "pch.h"
#include "InstallOptions.h"
#include "InstallOptions.g.cpp"





namespace winrt::Microsoft::Management::Deployment::implementation
{
    Microsoft::Management::Deployment::CatalogPackage InstallOptions::CatalogPackage()
    {
        return m_catalogPackage;
    }
    void InstallOptions::CatalogPackage(Microsoft::Management::Deployment::CatalogPackage const& value)
    {
        m_catalogPackage = value;
    }
    Microsoft::Management::Deployment::PackageVersionId InstallOptions::PackageVersionId()
    {
        throw hresult_not_implemented();
    }
    void InstallOptions::PackageVersionId(Microsoft::Management::Deployment::PackageVersionId const& value)
    {
        throw hresult_not_implemented();
    }
    hstring InstallOptions::PreferredInstallLocation()
    {
        throw hresult_not_implemented();
    }
    void InstallOptions::PreferredInstallLocation(hstring const& value)
    {
        throw hresult_not_implemented();
    }
    Microsoft::Management::Deployment::AppInstallScope InstallOptions::AppInstallScope()
    {
        return m_appInstallScope;
    }
    void InstallOptions::AppInstallScope(Microsoft::Management::Deployment::AppInstallScope const& value)
    {
        m_appInstallScope = value;
    }
    Microsoft::Management::Deployment::AppInstallMode InstallOptions::AppInstallMode()
    {
        return m_appInstallMode;
    }
    void InstallOptions::AppInstallMode(Microsoft::Management::Deployment::AppInstallMode const& value)
    {
        m_appInstallMode = value;
    }
    hstring InstallOptions::LogOutputPath()
    {
        throw hresult_not_implemented();
    }
    void InstallOptions::LogOutputPath(hstring const& value)
    {
        throw hresult_not_implemented();
    }
    bool InstallOptions::AllowHashMismatch()
    {
        throw hresult_not_implemented();
    }
    void InstallOptions::AllowHashMismatch(bool value)
    {
        throw hresult_not_implemented();
    }
    hstring InstallOptions::ReplacementInstallerArguments()
    {
        throw hresult_not_implemented();
    }
    void InstallOptions::ReplacementInstallerArguments(hstring const& value)
    {
        throw hresult_not_implemented();
    }
    hstring InstallOptions::AdditionalTelemetryArguments()
    {
        return hstring(m_additionalTelemetryArguments);
    }
    void InstallOptions::AdditionalTelemetryArguments(hstring const& value)
    {
        throw hresult_not_implemented();
    }
    hstring InstallOptions::AdditionalAppCatalogArguments()
    {
        throw hresult_not_implemented();
    }
    void InstallOptions::AdditionalAppCatalogArguments(hstring const& value)
    {
        throw hresult_not_implemented();
    }
}
