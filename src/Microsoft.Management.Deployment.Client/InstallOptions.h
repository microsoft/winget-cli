// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "InstallOptions.g.h"

const CLSID CLSID_InstallOptions2 = { 0x05F7019A, 0x8FAC, 0x4422, 0xBC, 0xD5, 0x4C, 0xB3, 0x4F, 0xFB, 0x44, 0xA8 };  //05F7019A-8FAC-4422-BCD5-4CB34FFB44A8

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct InstallOptions : InstallOptionsT<InstallOptions>
    {
        InstallOptions()
        {
            m_installOptions = winrt::create_instance<winrt::Microsoft::Management::Deployment::InstallOptions>(CLSID_InstallOptions2, CLSCTX_ALL);
        }

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
    private:
        winrt::Microsoft::Management::Deployment::InstallOptions m_installOptions{ nullptr };
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct InstallOptions : InstallOptionsT<InstallOptions, implementation::InstallOptions>
    {
    };
}
