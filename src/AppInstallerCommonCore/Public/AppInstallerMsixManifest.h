// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <AppxPackaging.h>
#include <AppInstallerStrings.h>
#include <wrl/client.h>

#include <string>

namespace AppInstaller::Msix
{
    using namespace Microsoft::WRL;
    using string_t = Utility::NormalizedString;

    namespace Platform
    {
        constexpr auto& WindowsDesktop = "windows.desktop";
        constexpr auto& WindowsUniversal = "windows.universal";

        bool IsWindowsDesktop(std::string platformName);
        bool IsWindowsUniversal(std::string platformName);
    }

    struct MsixPackageManifestIdentity
    {
        string_t PackageFamilyName;
        UINT64 Version = 0;
    };

    struct MsixPackageManifestTargetDeviceFamily
    {
        string_t Name;
        UINT64 MinVersion;
        UINT64 MaxVersionTested;

        MsixPackageManifestTargetDeviceFamily(string_t name, UINT64 minVersion, UINT64 maxVersionTested)
            : Name(name), MinVersion(minVersion), MaxVersionTested(maxVersionTested) {}
    };

    struct MsixPackageManifestDependencies
    {
        std::vector<MsixPackageManifestTargetDeviceFamily> TargetDeviceFamilies;
    };

    struct MsixPackageManifest
    {
        MsixPackageManifestDependencies Dependencies;
        MsixPackageManifestIdentity Identity;

        MsixPackageManifest() = default;
        MsixPackageManifest(ComPtr<IAppxManifestReader> manifestReader);
        void Assign(ComPtr<IAppxManifestReader> manifestReader);
    };
}