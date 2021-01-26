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

        // Required
        string_t Id;

        // Required
        string_t Version;

        ManifestVer ManifestVersion;

        ManifestInstaller DefaultInstallerInfo;

        std::vector<ManifestInstaller> Installers;

        ManifestLocalization DefaultLocalization;

        std::vector<ManifestLocalization> Localizations;

        ManifestLocalization CurrentLocalization;
    };
}