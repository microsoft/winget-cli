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

        string_t Moniker;

        ManifestVer ManifestVersion;

        ManifestInstaller DefaultInstallerInfo;

        std::vector<ManifestInstaller> Installers;

        ManifestLocalization DefaultLocalization;

        std::vector<ManifestLocalization> Localizations;

        ManifestLocalization CurrentLocalization;

        // If locale is empty, user setting locale will be used
        void ApplyLocale(const std::string& locale = {});

        std::vector<string_t> GetAggregatedTags() const;
        std::vector<string_t> GetAggregatedCommands() const;
    };
}