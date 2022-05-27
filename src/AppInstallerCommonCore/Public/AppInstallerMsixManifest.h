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

    namespace Platform
    {
        constexpr auto& WindowsDesktop = "windows.desktop";
        constexpr auto& WindowsUniversal = "windows.universal";

        bool IsWindowsDesktop(std::string platformName);
        bool IsWindowsUniversal(std::string platformName);
    }

    // Four parts version number: 16-bits.16-bits.16-bits.16-bits
    struct FourPartsVersionNumber : public Utility::Version
    {
        FourPartsVersionNumber() = default;
        FourPartsVersionNumber(std::string version) : Version(version) {}
        FourPartsVersionNumber(UINT64 version);

        UINT64 Major() const { return m_parts.size() > 0 ? m_parts[0].Integer : 0; }
        UINT64 Minor() const { return m_parts.size() > 1 ? m_parts[1].Integer : 0; }
        UINT64 Build() const { return m_parts.size() > 2 ? m_parts[2].Integer : 0; }
        UINT64 Revision() const { return m_parts.size() > 3 ? m_parts[3].Integer : 0; }
    };

    typedef FourPartsVersionNumber PackageVersion;
    typedef FourPartsVersionNumber OSVersion;

    struct MsixPackageManifestIdentity
    {
        string_t PackageFamilyName;
        PackageVersion Version;
    };

    struct MsixPackageManifestTargetDeviceFamily
    {
        string_t Name;
        OSVersion MinVersion;
        OSVersion MaxVersionTested;

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

    class MsixPackageManifestManager
    {
        std::map<std::string, std::vector<MsixPackageManifest>> m_msixManifests;
    public:
        const std::vector<MsixPackageManifest>& GetAppPackageManifests(std::string url);
    };
}