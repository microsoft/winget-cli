// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerStrings.h>
#include <AppInstallerSHA256.h>
#include <AppInstallerVersions.h>
#include <winget/ManifestInstaller.h>
#include <winget/ManifestLocalization.h>

#include <vector>

namespace AppInstaller::Manifest
{
    // Representation of the parsed manifest file.
    struct Manifest
    {
        using string_t = Utility::NormalizedString;

        string_t Id;

        string_t Version;

        string_t Channel;

        string_t Moniker;

        ManifestVer ManifestVersion;

        ManifestInstaller DefaultInstallerInfo;

        std::vector<ManifestInstaller> Installers;

        ManifestLocalization DefaultLocalization;

        std::vector<ManifestLocalization> Localizations;

        ManifestLocalization CurrentLocalization;

        // ApplyLocale will update the CurrentLocalization according to the specified locale
        // If locale is empty, user setting locale will be used
        void ApplyLocale(const std::string& locale = {});

        // Get all tags across localizations
        std::vector<string_t> GetAggregatedTags() const;

        // Get all commands across installers
        std::vector<string_t> GetAggregatedCommands() const;

        // Gets ARP version range if declared, otherwise an empty range is returned
        Utility::VersionRange GetArpVersionRange() const;

        // Get package family names across installers, Case folded.
        std::vector<string_t> GetPackageFamilyNames() const;

        // Get product codes across installers, Case folded.
        std::vector<string_t> GetProductCodes() const;

        // Get upgrade codes across installers, Case folded.
        std::vector<string_t> GetUpgradeCodes() const;

        // Get package names across localizations and installers, Case folded.
        std::vector<string_t> GetPackageNames() const;

        // Get publishers across localizations and installers, Case folded.
        std::vector<string_t> GetPublishers() const;

        // If not empty, the SHA256 hash of the manifest stream itself.
        Utility::SHA256::HashBuffer StreamSha256;

    private:
        std::vector<string_t> GetSystemReferenceStrings(
            std::function<const string_t& (const ManifestInstaller&)> extractStringFromInstaller = {},
            std::function<const string_t& (const AppsAndFeaturesEntry&)> extractStringFromAppsAndFeaturesEntry = {}) const;
    };
}