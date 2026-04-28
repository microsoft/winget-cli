// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "DownloadOptions.g.h"
#include "Public/ComClsids.h"
#include <winget/ModuleCountBase.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid(WINGET_OUTOFPROC_COM_CLSID_DownloadOptions)]
    struct DownloadOptions : DownloadOptionsT<DownloadOptions>
    {
        DownloadOptions();

        winrt::Microsoft::Management::Deployment::PackageVersionId PackageVersionId();
        void PackageVersionId(winrt::Microsoft::Management::Deployment::PackageVersionId const& value);
        winrt::Microsoft::Management::Deployment::PackageInstallScope Scope();
        void Scope(winrt::Microsoft::Management::Deployment::PackageInstallScope const& value);
        winrt::Microsoft::Management::Deployment::PackageInstallerType InstallerType();
        void InstallerType(winrt::Microsoft::Management::Deployment::PackageInstallerType const& value);
        winrt::Windows::System::ProcessorArchitecture Architecture();
        void Architecture(winrt::Windows::System::ProcessorArchitecture const& value);
        hstring Locale();
        void Locale(hstring const& value);
        hstring DownloadDirectory();
        void DownloadDirectory(hstring const& value);
        bool AllowHashMismatch();
        void AllowHashMismatch(bool value);
        bool SkipDependencies();
        void SkipDependencies(bool value);
        bool AcceptPackageAgreements();
        void AcceptPackageAgreements(bool value);
        hstring CorrelationData();
        void CorrelationData(hstring const& value);
        winrt::Microsoft::Management::Deployment::AuthenticationArguments AuthenticationArguments();
        void AuthenticationArguments(winrt::Microsoft::Management::Deployment::AuthenticationArguments const& value);
        bool SkipMicrosoftStoreLicense();
        void SkipMicrosoftStoreLicense(bool value);
        winrt::Microsoft::Management::Deployment::WindowsPlatform Platform();
        void Platform(winrt::Microsoft::Management::Deployment::WindowsPlatform value);
        hstring TargetOSVersion();
        void TargetOSVersion(hstring const& value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::PackageVersionId m_packageVersionId{ nullptr };
        winrt::Microsoft::Management::Deployment::PackageInstallScope m_scope = winrt::Microsoft::Management::Deployment::PackageInstallScope::Any;
        winrt::Microsoft::Management::Deployment::PackageInstallerType m_installerType = winrt::Microsoft::Management::Deployment::PackageInstallerType::Unknown;
        winrt::Windows::System::ProcessorArchitecture m_architecture = winrt::Windows::System::ProcessorArchitecture::Unknown;
        std::wstring m_locale = L"";
        std::wstring m_downloadDirectory = L"";
        bool m_allowHashMismatch = false;
        bool m_skipDependencies = false;
        bool m_acceptPackageAgreements = true;
        std::wstring m_correlationData = L"";
        winrt::Microsoft::Management::Deployment::AuthenticationArguments m_authenticationArguments{ nullptr };
        bool m_skipMicrosoftStoreLicense = false;
        winrt::Microsoft::Management::Deployment::WindowsPlatform m_platform = winrt::Microsoft::Management::Deployment::WindowsPlatform::Unknown;
        std::wstring m_targetOSVersion;
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct DownloadOptions : DownloadOptionsT<DownloadOptions, implementation::DownloadOptions>, AppInstaller::WinRT::ModuleCountBase
    {
    };
}
#endif
