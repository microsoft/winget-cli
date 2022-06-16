// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
#include <InstallOptions.h>
#include <Client.InstallOptions.h>
#pragma warning( pop )
#include "InstallOptions.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    InstallOptions::InstallOptions()
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::PackageVersionId InstallOptions::PackageVersionId()
    {
        throw hresult_not_implemented();
    }
    void InstallOptions::PackageVersionId(winrt::Microsoft::Management::Deployment::PackageVersionId const&)
    {
        throw hresult_not_implemented();
    }
    hstring InstallOptions::PreferredInstallLocation()
    {
        throw hresult_not_implemented();
    }
    void InstallOptions::PreferredInstallLocation(hstring const&)
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::PackageInstallScope InstallOptions::PackageInstallScope()
    {
        throw hresult_not_implemented();
    }
    void InstallOptions::PackageInstallScope(winrt::Microsoft::Management::Deployment::PackageInstallScope const&)
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::PackageInstallMode InstallOptions::PackageInstallMode()
    {
        throw hresult_not_implemented();
    }
    void InstallOptions::PackageInstallMode(winrt::Microsoft::Management::Deployment::PackageInstallMode const&)
    {
        throw hresult_not_implemented();
    }
    hstring InstallOptions::LogOutputPath()
    {
        throw hresult_not_implemented();
    }
    void InstallOptions::LogOutputPath(hstring const&)
    {
        throw hresult_not_implemented();
    }
    bool InstallOptions::AllowHashMismatch()
    {
        throw hresult_not_implemented();
    }
    void InstallOptions::AllowHashMismatch(bool)
    {
        throw hresult_not_implemented();
    }
    hstring InstallOptions::ReplacementInstallerArguments()
    {
        throw hresult_not_implemented();
    }
    void InstallOptions::ReplacementInstallerArguments(hstring const&)
    {
        throw hresult_not_implemented();
    }
    hstring InstallOptions::CorrelationData()
    {
        throw hresult_not_implemented();
    }
    void InstallOptions::CorrelationData(hstring const&)
    {
        throw hresult_not_implemented();
    }
    hstring InstallOptions::AdditionalPackageCatalogArguments()
    {
        throw hresult_not_implemented();
    }
    void InstallOptions::AdditionalPackageCatalogArguments(hstring const&)
    {
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::Collections::IVector<winrt::Windows::System::ProcessorArchitecture> InstallOptions::AllowedArchitectures()
    {
        throw hresult_not_implemented();
    }
    bool InstallOptions::AllowUpgradeToUnknownVersion()
    {
        throw hresult_not_implemented();
    }
    void InstallOptions::AllowUpgradeToUnknownVersion(bool)
    {
        throw hresult_not_implemented();
    }
    bool InstallOptions::IgnoreInstallWarnings()
    {
        throw hresult_not_implemented();
    }
    void InstallOptions::IgnoreInstallWarnings(bool)
    {
        throw hresult_not_implemented();
    }
}
