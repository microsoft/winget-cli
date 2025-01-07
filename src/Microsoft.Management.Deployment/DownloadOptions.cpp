// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 6388 Allow CreateInstance.
#include <wil\cppwinrt_wrl.h>
// 4467 Allow use of uuid attribute for com object creation.
#include "DownloadOptions.h"
#pragma warning( pop )
#include "DownloadOptions.g.cpp"
#include "Helpers.h"

#include <AppInstallerArchitecture.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    DownloadOptions::DownloadOptions()
    {
    }
    winrt::Microsoft::Management::Deployment::PackageVersionId DownloadOptions::PackageVersionId()
    {
        return m_packageVersionId;
    }
    void DownloadOptions::PackageVersionId(winrt::Microsoft::Management::Deployment::PackageVersionId const& value)
    {
        m_packageVersionId = value;
    }
    winrt::Microsoft::Management::Deployment::PackageInstallScope DownloadOptions::Scope()
    {
        return m_scope;
    }
    void DownloadOptions::Scope(winrt::Microsoft::Management::Deployment::PackageInstallScope const& value)
    {
        m_scope = value;
    }
    winrt::Microsoft::Management::Deployment::PackageInstallerType DownloadOptions::InstallerType()
    {
        return m_installerType;
    }
    void DownloadOptions::InstallerType(winrt::Microsoft::Management::Deployment::PackageInstallerType const& value)
    {
        m_installerType = value;
    }
    winrt::Windows::System::ProcessorArchitecture DownloadOptions::Architecture()
    {
        return m_architecture;
    }
    void DownloadOptions::Architecture(winrt::Windows::System::ProcessorArchitecture const& value)
    {
        m_architecture = value;
    }
    hstring DownloadOptions::Locale()
    {
        return hstring(m_locale);
    }
    void DownloadOptions::Locale(hstring const& value)
    {
        m_locale = value;
    }
    hstring DownloadOptions::DownloadDirectory()
    {
        return hstring(m_downloadDirectory);
    }
    void DownloadOptions::DownloadDirectory(hstring const& value)
    {
        m_downloadDirectory = value;
    }
    bool DownloadOptions::AllowHashMismatch()
    {
        return m_allowHashMismatch;
    }
    void DownloadOptions::AllowHashMismatch(bool value)
    {
        m_allowHashMismatch = value;
    }
    bool DownloadOptions::SkipDependencies()
    {
        return m_skipDependencies;
    }
    void DownloadOptions::SkipDependencies(bool value)
    {
        m_skipDependencies = value;
    }
    bool DownloadOptions::AcceptPackageAgreements()
    {
        return m_acceptPackageAgreements;
    }
    void DownloadOptions::AcceptPackageAgreements(bool value)
    {
        m_acceptPackageAgreements = value;
    }
    hstring DownloadOptions::CorrelationData()
    {
        return hstring(m_correlationData);
    }
    void DownloadOptions::CorrelationData(hstring const& value)
    {
        m_correlationData = value;
    }
    winrt::Microsoft::Management::Deployment::AuthenticationArguments DownloadOptions::AuthenticationArguments()
    {
        return m_authenticationArguments;
    }
    void DownloadOptions::AuthenticationArguments(winrt::Microsoft::Management::Deployment::AuthenticationArguments const& value)
    {
        m_authenticationArguments = value;
    }
    CoCreatableMicrosoftManagementDeploymentClass(DownloadOptions);
}
