// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionArgs.h"
#include <winget/Manifest.h>

#include <optional>


namespace AppInstaller::CLI::Workflow
{
    // This is used in sorting the list of available installers to get the best match.
    struct InstallerComparator
    {
        InstallerComparator(const std::map<std::string, std::string>& installationMetadata) :
            m_installationMetadata(installationMetadata) {}

        bool operator() (
            const Manifest::ManifestInstaller& installer1,
            const Manifest::ManifestInstaller& installer2);

    private:
        const std::map<std::string, std::string>& m_installationMetadata;
    };

    // This is used in sorting the list of available localizations to get the best match.
    struct LocalizationComparator
    {
        bool operator() (
            const Manifest::ManifestLocalization& loc1,
            const Manifest::ManifestLocalization& loc2);
    };

    // Class in charge of comparing manifest entries
    struct ManifestComparator
    {
        ManifestComparator(const Execution::Args&, const std::map<std::string, std::string>& installationMetadata = {}) :
            m_installationMetadata(installationMetadata), m_installerComparator(installationMetadata) {}

        std::optional<Manifest::ManifestInstaller> GetPreferredInstaller(const Manifest::Manifest& manifest);
        Manifest::ManifestLocalization GetPreferredLocalization(const Manifest::Manifest& manifest);

    private:
        // TODO: Handle args to change how we select.
        const std::map<std::string, std::string>& m_installationMetadata;
        LocalizationComparator m_localizationComparator;
        InstallerComparator m_installerComparator;
    };

}