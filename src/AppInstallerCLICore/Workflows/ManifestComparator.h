// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

namespace AppInstaller::Workflow
{
    // This is used in sorting the list of available installers to get the best match.
    struct InstallerComparator
    {
        bool operator() (
            const AppInstaller::Manifest::ManifestInstaller& installer1,
            const AppInstaller::Manifest::ManifestInstaller& installer2);
    };

    // This is used in sorting the list of available localizations to get the best match.
    struct LocalizationComparator
    {
        bool operator() (
            const AppInstaller::Manifest::ManifestLocalization& loc1,
            const AppInstaller::Manifest::ManifestLocalization& loc2);
    };

    // Class in charge of comparing manifest entries
    class ManifestComparator
    {
    public:
        ManifestComparator(AppInstaller::Manifest::Manifest& manifest, WorkflowReporter& reporter) : m_manifestRef(manifest), m_reporterRef(reporter) {}

        AppInstaller::Manifest::ManifestInstaller GetPreferredInstaller(const std::locale& preferredLocale);
        AppInstaller::Manifest::ManifestLocalization GetPreferredLocalization(const std::locale& preferredLocale);

    private:
        AppInstaller::Manifest::Manifest& m_manifestRef;
        WorkflowReporter& m_reporterRef;
    };

}