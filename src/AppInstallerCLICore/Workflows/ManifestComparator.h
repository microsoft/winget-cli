// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionArgs.h"
#include <Manifest/Manifest.h>

#include <optional>


namespace AppInstaller::CLI::Workflow
{
    // This is used in sorting the list of available installers to get the best match.
    struct InstallerComparator
    {
        bool operator() (
            const Manifest::ManifestInstaller& installer1,
            const Manifest::ManifestInstaller& installer2);
    };

    // This is used in sorting the list of available localizations to get the best match.
    struct LocalizationComparator
    {
        bool operator() (
            const Manifest::ManifestLocalization& loc1,
            const Manifest::ManifestLocalization& loc2);
    };

    // Class in charge of comparing manifest entries
    class ManifestComparator
    {
    public:
        ManifestComparator(const Execution::Args&) {}

        std::optional<Manifest::ManifestInstaller> GetPreferredInstaller(const Manifest::Manifest& manifest);
        Manifest::ManifestLocalization GetPreferredLocalization(const Manifest::Manifest& manifest);

    private:
        // TODO: Handle args to change how we select.
    };

}