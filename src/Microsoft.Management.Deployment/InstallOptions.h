// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "InstallOptions.g.h"
#include "Public/ComClsids.h"
#include <winget/ModuleCountBase.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid(WINGET_OUTOFPROC_COM_CLSID_InstallOptions)]
    struct InstallOptions : InstallOptionsT<InstallOptions>
    {
        InstallOptions();

        winrt::Microsoft::Management::Deployment::PackageVersionId PackageVersionId();
        void PackageVersionId(winrt::Microsoft::Management::Deployment::PackageVersionId const& value);
        hstring PreferredInstallLocation();
        void PreferredInstallLocation(hstring const& value);
        winrt::Microsoft::Management::Deployment::PackageInstallScope PackageInstallScope();
        void PackageInstallScope(winrt::Microsoft::Management::Deployment::PackageInstallScope const& value);
        winrt::Microsoft::Management::Deployment::PackageInstallMode PackageInstallMode();
        void PackageInstallMode(winrt::Microsoft::Management::Deployment::PackageInstallMode const& value);
        winrt::Microsoft::Management::Deployment::PackageInstallerType InstallerType();
        void InstallerType(winrt::Microsoft::Management::Deployment::PackageInstallerType const& value);
        hstring LogOutputPath();
        void LogOutputPath(hstring const& value);
        bool AllowHashMismatch();
        void AllowHashMismatch(bool value);
        bool BypassIsStoreClientBlockedPolicyCheck();
        void BypassIsStoreClientBlockedPolicyCheck(bool value);
        hstring ReplacementInstallerArguments();
        void ReplacementInstallerArguments(hstring const& value);
        hstring AdditionalInstallerArguments();
        void AdditionalInstallerArguments(hstring const& value);
        hstring CorrelationData();
        void CorrelationData(hstring const& value);
        hstring AdditionalPackageCatalogArguments();
        void AdditionalPackageCatalogArguments(hstring const& value);
        winrt::Windows::Foundation::Collections::IVector<winrt::Windows::System::ProcessorArchitecture> AllowedArchitectures();
        bool AllowUpgradeToUnknownVersion();
        void AllowUpgradeToUnknownVersion(bool value);
        bool Force();
        void Force(bool value);
        bool AcceptPackageAgreements();
        void AcceptPackageAgreements(bool value);
        bool SkipDependencies();
        void SkipDependencies(bool value);
        winrt::Microsoft::Management::Deployment::AuthenticationArguments AuthenticationArguments();
        void AuthenticationArguments(winrt::Microsoft::Management::Deployment::AuthenticationArguments const& value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::PackageVersionId m_packageVersionId{ nullptr };
        std::wstring m_preferredInstallLocation = L"";
        winrt::Microsoft::Management::Deployment::PackageInstallScope m_packageInstallScope = winrt::Microsoft::Management::Deployment::PackageInstallScope::Any;
        winrt::Microsoft::Management::Deployment::PackageInstallMode m_packageInstallMode = winrt::Microsoft::Management::Deployment::PackageInstallMode::Default;
        winrt::Microsoft::Management::Deployment::PackageInstallerType m_installerType = winrt::Microsoft::Management::Deployment::PackageInstallerType::Unknown;
        std::wstring m_logOutputPath = L"";
        bool m_allowHashMismatch = false;
        bool m_bypassIsStoreClientBlockedPolicyCheck = false;
        std::wstring m_replacementInstallerArguments = L"";
        std::wstring m_additionalInstallerArguments = L"";
        std::wstring m_correlationData = L"";
        std::wstring m_additionalPackageCatalogArguments = L"";
        Windows::Foundation::Collections::IVector<Windows::System::ProcessorArchitecture> m_allowedArchitectures{
            winrt::single_threaded_vector<winrt::Windows::System::ProcessorArchitecture>() };
        bool m_allowUpgradeToUnknownVersion = false;
        bool m_force = false;
        bool m_acceptPackageAgreements = true;
        bool m_skipDependencies = false;
        winrt::Microsoft::Management::Deployment::AuthenticationArguments m_authenticationArguments{ nullptr };
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct InstallOptions : InstallOptionsT<InstallOptions, implementation::InstallOptions>, AppInstaller::WinRT::ModuleCountBase
    {
    };
}
#endif
