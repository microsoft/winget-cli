// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 6388 Allow CreateInstance.
#include <wil\cppwinrt_wrl.h>
// 4467 Allow use of uuid attribute for com object creation.
#include "InstallOptions.h"
#pragma warning( pop )
#include "InstallOptions.g.cpp"
#include "Converters.h"
#include "Helpers.h"

#include <AppInstallerArchitecture.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    InstallOptions::InstallOptions()
    {
        // Populate the allowed architectures with the default values for the machine
        for (AppInstaller::Utility::Architecture architecture : AppInstaller::Utility::GetApplicableArchitectures())
        {
            auto convertedArchitecture = GetWindowsSystemProcessorArchitecture(architecture);
            if (convertedArchitecture)
            {
                m_allowedArchitectures.Append(convertedArchitecture.value());
            }
        }
    }
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
    winrt::Microsoft::Management::Deployment::PackageInstallScope InstallOptions::PackageInstallScope()
    {
        return m_packageInstallScope;
    }
    void InstallOptions::PackageInstallScope(winrt::Microsoft::Management::Deployment::PackageInstallScope const& value)
    {
        m_packageInstallScope = value;
    }
    winrt::Microsoft::Management::Deployment::PackageInstallMode InstallOptions::PackageInstallMode()
    {
        return m_packageInstallMode;
    }
    void InstallOptions::PackageInstallMode(winrt::Microsoft::Management::Deployment::PackageInstallMode const& value)
    {
        m_packageInstallMode = value;
    }
    winrt::Microsoft::Management::Deployment::PackageInstallerType InstallOptions::InstallerType()
    {
        return m_installerType;
    }
    void InstallOptions::InstallerType(winrt::Microsoft::Management::Deployment::PackageInstallerType const& value)
    {
        m_installerType = value;
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
    bool InstallOptions::BypassIsStoreClientBlockedPolicyCheck()
    {
        return m_bypassIsStoreClientBlockedPolicyCheck;
    }
    void InstallOptions::BypassIsStoreClientBlockedPolicyCheck(bool value)
    {
        m_bypassIsStoreClientBlockedPolicyCheck = value;
    }
    hstring InstallOptions::ReplacementInstallerArguments()
    {
        return hstring(m_replacementInstallerArguments);
    }
    void InstallOptions::ReplacementInstallerArguments(hstring const& value)
    {
        m_replacementInstallerArguments = value;
    }
    hstring InstallOptions::AdditionalInstallerArguments()
    {
        return hstring(m_additionalInstallerArguments);
    }
    void InstallOptions::AdditionalInstallerArguments(hstring const& value)
    {
        m_additionalInstallerArguments = value;
    }
    hstring InstallOptions::CorrelationData()
    {
        return hstring(m_correlationData);
    }
    void InstallOptions::CorrelationData(hstring const& value)
    {
        m_correlationData = value;
    }
    hstring InstallOptions::AdditionalPackageCatalogArguments()
    {
        return hstring(m_additionalPackageCatalogArguments);
    }
    void InstallOptions::AdditionalPackageCatalogArguments(hstring const& value)
    {
        m_additionalPackageCatalogArguments = value;
    }
    winrt::Windows::Foundation::Collections::IVector<winrt::Windows::System::ProcessorArchitecture> InstallOptions::AllowedArchitectures()
    {
        return m_allowedArchitectures;
    }
    bool InstallOptions::AllowUpgradeToUnknownVersion()
    {
        return m_allowUpgradeToUnknownVersion;
    }
    void InstallOptions::AllowUpgradeToUnknownVersion(bool value)
    {
        m_allowUpgradeToUnknownVersion = value;
    }
    bool InstallOptions::Force()
    {
        return m_force;
    }
    void InstallOptions::Force(bool value)
    {
        m_force = value;
    }
    void InstallOptions::AcceptPackageAgreements(bool value)
    {
        m_acceptPackageAgreements = value;
    }
    bool InstallOptions::AcceptPackageAgreements()
    {
        return m_acceptPackageAgreements;
    }
    void InstallOptions::SkipDependencies(bool value)
    {
        m_skipDependencies = value;
    }
    bool InstallOptions::SkipDependencies()
    {
        return m_skipDependencies;
    }
    winrt::Microsoft::Management::Deployment::AuthenticationArguments InstallOptions::AuthenticationArguments()
    {
        return m_authenticationArguments;
    }
    void InstallOptions::AuthenticationArguments(winrt::Microsoft::Management::Deployment::AuthenticationArguments const& value)
    {
        m_authenticationArguments = value;
    }
    CoCreatableMicrosoftManagementDeploymentClass(InstallOptions);
}
