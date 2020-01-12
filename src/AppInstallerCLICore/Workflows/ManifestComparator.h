#pragma once

namespace AppInstaller::Workflow
{
    struct InstallerComparator
    {
        bool operator() (
            const AppInstaller::Manifest::ManifestInstaller& struct1,
            const AppInstaller::Manifest::ManifestInstaller& struct2);
    };

    struct LocalizationComparator
    {
        bool operator() (
            const AppInstaller::Manifest::ManifestLocalization& struct1,
            const AppInstaller::Manifest::ManifestLocalization& struct2);
    };
}