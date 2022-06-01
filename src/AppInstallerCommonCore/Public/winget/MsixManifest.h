// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <AppxPackaging.h>
#include <AppInstallerStrings.h>
#include <AppInstallerVersions.h>
#include <wrl/client.h>

#include <string>

namespace AppInstaller::Msix
{
    using namespace Microsoft::WRL;
    using string_t = Utility::NormalizedString;

    typedef Utility::FourPartsVersionNumber PackageVersion;
    typedef Utility::FourPartsVersionNumber OSVersion;

    // Package identity for a msix manifest
    struct MsixPackageManifestIdentity
    {
        MsixPackageManifestIdentity(ComPtr<IAppxManifestPackageId> packageId);

        string_t GetPackageFamilyName() const;
        PackageVersion GetVersion() const;
    private:
        ComPtr<IAppxManifestPackageId> m_packageId;
    };

    // Target device family for a msix manifest
    struct MsixPackageManifestTargetDeviceFamily
    {
        MsixPackageManifestTargetDeviceFamily(ComPtr<IAppxManifestTargetDeviceFamily> targetDeviceFamily);

        string_t GetName() const;
        OSVersion GetMinVersion() const;
    private:
        ComPtr<IAppxManifestTargetDeviceFamily> m_targetDeviceFamily;
    };

    // Msix manifest
    struct MsixPackageManifest
    {
        MsixPackageManifest(ComPtr<IAppxManifestReader> manifestReader);

        std::vector<MsixPackageManifestTargetDeviceFamily> GetTargetDeviceFamilies() const;
        MsixPackageManifestIdentity GetIdentity() const;
        std::optional<OSVersion> GetMinimumOSVersion() const;
    private:
        ComPtr<IAppxManifestReader> m_manifestReader;
    };

    class MsixPackageManifestManager
    {
        std::map<std::string, std::vector<MsixPackageManifest>> m_msixManifests;
    public:
        const std::vector<MsixPackageManifest>& GetAppPackageManifests(std::string url);
    };
}