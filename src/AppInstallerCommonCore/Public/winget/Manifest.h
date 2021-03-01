// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerStrings.h>
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
    };
}