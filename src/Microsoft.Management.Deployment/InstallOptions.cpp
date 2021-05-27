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
#include "Helpers.h"

const GUID InstallOptionsCLSID1 = { 0x1095F097, 0xEB96, 0x453B, 0xB4, 0xE6, 0x16, 0x13, 0x63, 0x7F, 0x3B, 0x14 };  //1095F097-EB96-453B-B4E6-1613637F3B14
const GUID InstallOptionsCLSID2 = { 0x05F7019A, 0x8FAC, 0x4422, 0xBC, 0xD5, 0x4C, 0xB3, 0x4F, 0xFB, 0x44, 0xA8 };  //05F7019A-8FAC-4422-BCD5-4CB34FFB44A8

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
    hstring InstallOptions::AdditionalPackageCatalogArguments()
    {
        return hstring(m_additionalPackageCatalogArguments);
    }
    void InstallOptions::AdditionalPackageCatalogArguments(hstring const& value)
    {
        m_additionalPackageCatalogArguments = value;
    }
    CoCreatableCppWinRtClassWithCLSID(InstallOptions, 1, &InstallOptionsCLSID1);
    CoCreatableCppWinRtClassWithCLSID(InstallOptions, 2, &InstallOptionsCLSID2);
}
