// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerStrings.h>
#include <AppInstallerVersions.h>
#include <winget/ManifestInstaller.h>
#include <winget/ManifestLocalization.h>

#include <vector>

namespace AppInstaller::Manifest
{
    // ManifestVer is inherited from Utility::Version and is a more restricted version.
    // ManifestVer is used to specify the version of app manifest itself.
    // ManifestVer is a 3 part version in the format of [0-65535].[0-65535].[0-65535]
    // and optionally a following tag in the format of -[SomeString] for experimental purpose.
    struct ManifestVer : public Utility::Version
    {
        ManifestVer() = default;

        ManifestVer(std::string_view version);

        uint64_t Major() const { return m_parts.size() > 0 ? m_parts[0].Integer : 0; }
        uint64_t Minor() const { return m_parts.size() > 1 ? m_parts[1].Integer : 0; }
        uint64_t Patch() const { return m_parts.size() > 2 ? m_parts[2].Integer : 0; }

        bool HasExtension() const;

        bool HasExtension(std::string_view extension) const;

    private:
        std::vector<Version> m_extensions;
    };

    // Representation of the parsed manifest file.
    struct Manifest
    {
        using string_t = Utility::NormalizedString;

        // Required
        string_t Id;

        // Required
        string_t Name;

        // Required
        string_t Version;

        // Required
        string_t Publisher;

        string_t AppMoniker;

        string_t Channel;

        string_t Author;

        string_t License;

        string_t MinOSVersion;

        // Comma separated values
        std::vector<string_t> Tags;

        // Comma separated values
        std::vector<string_t> Commands;

        // Comma separated values
        std::vector<string_t> Protocols;

        // Comma separated values
        std::vector<string_t> FileExtensions;

        ManifestInstaller::InstallerTypeEnum InstallerType = ManifestInstaller::InstallerTypeEnum::Unknown;

        // Default is Install if not specified
        ManifestInstaller::UpdateBehaviorEnum UpdateBehavior = ManifestInstaller::UpdateBehaviorEnum::Install;

        // Package family name for MSIX packaged installers.
        string_t PackageFamilyName;

        // Product code for ARP (Add/Remove Programs) installers.
        string_t ProductCode;

        string_t Description;

        string_t Homepage;

        string_t LicenseUrl;

        ManifestVer ManifestVersion;

        std::map<ManifestInstaller::InstallerSwitchType, string_t> Switches;

        std::vector<ManifestInstaller> Installers;

        std::vector<ManifestLocalization> Localization;
    };
}