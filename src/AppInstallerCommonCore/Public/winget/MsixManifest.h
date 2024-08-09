// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "AppInstallerStrings.h"
#include "AppInstallerVersions.h"

#include <AppxPackaging.h>
#include <wrl/client.h>

namespace AppInstaller::Msix
{
    using PackageVersion = Utility::UInt64Version;
    using OSVersion = Utility::UInt64Version;

    // Package identity for an MSIX manifest
    struct MsixPackageManifestIdentity
    {
        MsixPackageManifestIdentity(Microsoft::WRL::ComPtr<IAppxManifestPackageId> packageId)
            : m_packageId(packageId) {}

        Utility::NormalizedString GetPackageFamilyName() const;
        PackageVersion GetVersion() const;
    private:
        Microsoft::WRL::ComPtr<IAppxManifestPackageId> m_packageId;
    };

    // Target device family for an MSIX manifest
    struct MsixPackageManifestTargetDeviceFamily
    {
        // Target device family names
        static constexpr std::string_view WindowsDesktopName = "Windows.Desktop";
        static constexpr std::string_view WindowsUniversalName = "Windows.Universal";

        enum Platform
        {
            WindowsDesktop,
            WindowsUniversal,
            Other,
        };

        MsixPackageManifestTargetDeviceFamily(Microsoft::WRL::ComPtr<IAppxManifestTargetDeviceFamily> targetDeviceFamily)
            : m_targetDeviceFamily(targetDeviceFamily) {}

        std::string GetName() const;
        OSVersion GetMinVersion() const;
        Platform GetPlatform() const;
    private:
        Microsoft::WRL::ComPtr<IAppxManifestTargetDeviceFamily> m_targetDeviceFamily;
    };

    // MSIX manifest
    struct MsixPackageManifest
    {
        MsixPackageManifest(Microsoft::WRL::ComPtr<IAppxManifestReader> manifestReader)
            : m_manifestReader(manifestReader) {}

        std::vector<MsixPackageManifestTargetDeviceFamily> GetTargetDeviceFamilies() const;
        MsixPackageManifestIdentity GetIdentity() const;
        std::optional<OSVersion> GetMinimumOSVersionForSupportedPlatforms() const;
    private:
        Microsoft::WRL::ComPtr<IAppxManifestReader> m_manifestReader;
    };
}