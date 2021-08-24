// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "InstallOptions.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
#if USE_PROD_CLSIDS 
    [uuid("1095F097-EB96-453B-B4E6-1613637F3B14")]
#else
    [uuid("44FE0580-62F7-44D4-9E91-AA9614AB3E86")]
#endif
    struct InstallOptions : InstallOptionsT<InstallOptions>
    {
        InstallOptions() = default;

        winrt::Microsoft::Management::Deployment::PackageVersionId PackageVersionId();
        void PackageVersionId(winrt::Microsoft::Management::Deployment::PackageVersionId const& value);
        hstring PreferredInstallLocation();
        void PreferredInstallLocation(hstring const& value);
        winrt::Microsoft::Management::Deployment::PackageInstallScope PackageInstallScope();
        void PackageInstallScope(winrt::Microsoft::Management::Deployment::PackageInstallScope const& value);
        winrt::Microsoft::Management::Deployment::PackageInstallMode PackageInstallMode();
        void PackageInstallMode(winrt::Microsoft::Management::Deployment::PackageInstallMode const& value);
        hstring LogOutputPath();
        void LogOutputPath(hstring const& value);
        bool AllowHashMismatch();
        void AllowHashMismatch(bool value);
        hstring ReplacementInstallerArguments();
        void ReplacementInstallerArguments(hstring const& value);
        hstring CorrelationData();
        void CorrelationData(hstring const& value);
        hstring AdditionalPackageCatalogArguments();
        void AdditionalPackageCatalogArguments(hstring const& value);
        winrt::Windows::Foundation::Collections::IVector<winrt::Windows::System::ProcessorArchitecture> AllowedArchitectures();
    private:
        winrt::Microsoft::Management::Deployment::PackageVersionId m_packageVersionId{ nullptr };
        std::wstring m_preferredInstallLocation = L"";
        winrt::Microsoft::Management::Deployment::PackageInstallScope m_packageInstallScope = winrt::Microsoft::Management::Deployment::PackageInstallScope::Any;
        winrt::Microsoft::Management::Deployment::PackageInstallMode m_packageInstallMode = winrt::Microsoft::Management::Deployment::PackageInstallMode::Default;
        std::wstring m_logOutputPath = L"";
        bool m_allowHashMismatch = false;
        std::wstring m_replacementInstallerArguments = L"";
        std::wstring m_correlationData = L"";
        std::wstring m_additionalPackageCatalogArguments = L"";
        Windows::Foundation::Collections::IVector<Windows::System::ProcessorArchitecture> m_allowedArchitectures{
            winrt::single_threaded_vector<winrt::Windows::System::ProcessorArchitecture>() };
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct InstallOptions : InstallOptionsT<InstallOptions, implementation::InstallOptions>
    {
    };
}
