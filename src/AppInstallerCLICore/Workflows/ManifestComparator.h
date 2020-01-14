// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

namespace AppInstaller::Workflow
{
    // This is used in sorting the list of available installers to get the best match.
    struct InstallerComparator
    {
        bool operator() (
            const AppInstaller::Manifest::ManifestInstaller& struct1,
            const AppInstaller::Manifest::ManifestInstaller& struct2);
    };

    // This is used in sorting the list of available localizations to get the best match.
    struct LocalizationComparator
    {
        bool operator() (
            const AppInstaller::Manifest::ManifestLocalization& struct1,
            const AppInstaller::Manifest::ManifestLocalization& struct2);
    };
}