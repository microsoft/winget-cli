#pragma once
#include "InstallOptions.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid("1095F097-EB96-453B-B4E6-1613637F3B14")]
    struct InstallOptions : InstallOptionsT<InstallOptions>
    {
        InstallOptions() = default;

        winrt::Microsoft::Management::Deployment::PackageVersionId PackageVersionId();
        void PackageVersionId(winrt::Microsoft::Management::Deployment::PackageVersionId const& value);
        hstring PreferredInstallLocation();
        void PreferredInstallLocation(hstring const& value);
        winrt::Microsoft::Management::Deployment::AppInstallScope AppInstallScope();
        void AppInstallScope(winrt::Microsoft::Management::Deployment::AppInstallScope const& value);
        winrt::Microsoft::Management::Deployment::AppInstallMode AppInstallMode();
        void AppInstallMode(winrt::Microsoft::Management::Deployment::AppInstallMode const& value);
        hstring LogOutputPath();
        void LogOutputPath(hstring const& value);
        bool AllowHashMismatch();
        void AllowHashMismatch(bool value);
        hstring ReplacementInstallerArguments();
        void ReplacementInstallerArguments(hstring const& value);
        hstring CorrelationData();
        void CorrelationData(hstring const& value);
        hstring AdditionalAppCatalogArguments();
        void AdditionalAppCatalogArguments(hstring const& value);
    private:
        winrt::Microsoft::Management::Deployment::PackageVersionId m_packageVersionId{ nullptr };
        std::wstring m_preferredInstallLocation = L"";
        winrt::Microsoft::Management::Deployment::AppInstallScope m_appInstallScope = winrt::Microsoft::Management::Deployment::AppInstallScope::User;
        winrt::Microsoft::Management::Deployment::AppInstallMode m_appInstallMode = winrt::Microsoft::Management::Deployment::AppInstallMode::Default;
        std::wstring m_logOutputPath = L"";
        bool m_allowHashMismatch = false;
        std::wstring m_replacementInstallerArguments = L"";
        std::wstring m_correlationData = L"";
        std::wstring m_additionalAppCatalogArguments = L"";
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct InstallOptions : InstallOptionsT<InstallOptions, implementation::InstallOptions>
    {
    };
}
