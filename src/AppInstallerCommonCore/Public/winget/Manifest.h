// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ManifestInstaller.h"
#include "ManifestLocalization.h"
#include <AppInstallerStrings.h>
#include <AppInstallerVersions.h>
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

        ManifestVer(std::string version, bool fullValidation);

        uint64_t Major() const { return m_parts.size() > 0 ? m_parts[0].Integer : 0; }
        uint64_t Minor() const { return m_parts.size() > 1 ? m_parts[1].Integer : 0; }
        uint64_t Patch() const { return m_parts.size() > 2 ? m_parts[2].Integer : 0; }

        bool HasTag() const;
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

        string_t Description;

        string_t Homepage;

        string_t LicenseUrl;

        ManifestVer ManifestVersion;

        std::map<ManifestInstaller::InstallerSwitchType, string_t> Switches;

        std::vector<ManifestInstaller> Installers;

        std::vector<ManifestLocalization> Localization;
    };
}