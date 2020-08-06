// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestComparator.h"

using namespace AppInstaller::CLI;
using namespace AppInstaller::Manifest;

namespace AppInstaller::CLI::Workflow
{
    bool InstallerComparator::operator() (const ManifestInstaller& installer1, const ManifestInstaller& installer2)
    {
        // Todo: Compare only architecture for now. Need more work and spec.
        if (Utility::IsApplicableArchitecture(installer1.Arch) > Utility::IsApplicableArchitecture(installer2.Arch))
        {
            return true;
        }

        return false;
    }

    bool LocalizationComparator::operator() (const ManifestLocalization& loc1, const ManifestLocalization& loc2)
    {
        // Todo: Compare simple language for now. Need more work and spec.
        std::string userPreferredLocale = std::locale("").name();

        auto foundLoc1 = userPreferredLocale.find(loc1.Language);
        auto foundLoc2 = userPreferredLocale.find(loc2.Language);

        if (foundLoc1 != std::string::npos && foundLoc2 == std::string::npos)
        {
            return true;
        }

        return false;
    }

    std::optional<Manifest::ManifestInstaller> ManifestComparator::GetPreferredInstaller(const Manifest::Manifest& manifest)
    {
        AICLI_LOG(CLI, Info, << "Starting installer selection.");

        // Sorting the list of available installers according to rules defined in InstallerComparator.
        auto installers = manifest.Installers;
        std::sort(installers.begin(), installers.end(), InstallerComparator());

        // If the first one is inapplicable, then no installer is applicable.
        if (Utility::IsApplicableArchitecture(installers[0].Arch) == -1)
        {
            return {};
        }

        ManifestInstaller& selectedInstaller = installers[0];

        Logging::Telemetry().LogSelectedInstaller((int)selectedInstaller.Arch, selectedInstaller.Url, Manifest::ManifestInstaller::InstallerTypeToString(selectedInstaller.InstallerType), selectedInstaller.Scope, selectedInstaller.Language);

        return std::move(selectedInstaller);
    }

    Manifest::ManifestLocalization ManifestComparator::GetPreferredLocalization(const Manifest::Manifest& manifest)
    {
        AICLI_LOG(CLI, Info, << "Starting localization selection.");

        ManifestLocalization selectedLocalization;

        // Sorting the list of available localizations according to rules defined in LocalizationComparator.
        if (!manifest.Localization.empty())
        {
            auto localization = manifest.Localization;
            std::sort(localization.begin(), localization.end(), LocalizationComparator());

            // TODO: needs to check language applicability here

            selectedLocalization = localization[0];
        }
        else
        {
            // Populate default from package manifest
            selectedLocalization.Description = manifest.Description;
            selectedLocalization.Homepage = manifest.Homepage;
            selectedLocalization.LicenseUrl = manifest.LicenseUrl;
        }

        AICLI_LOG(CLI, Info, << "Completed localization selection. Selected localization language: " << selectedLocalization.Language);

        return selectedLocalization;
    }
}