// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowBase.h"
#include "ManifestComparator.h"

using namespace AppInstaller::CLI;
using namespace AppInstaller::Manifest;

namespace AppInstaller::CLI::Workflow
{
    bool InstallerComparator::operator() (const ManifestInstaller& installer1, const ManifestInstaller& installer2)
    {
        // Applicable architecture should always come before inapplicable architecture
        if (Utility::IsApplicableArchitecture(installer1.Arch) != Utility::InapplicableArchitecture &&
            Utility::IsApplicableArchitecture(installer2.Arch) == Utility::InapplicableArchitecture)
        {
            return true;
        }

        // If there's installation metadata, pick the preferred one or compatible one
        auto installerTypeItr = m_installationMetadata.find(s_InstallationMetadata_Key_InstallerType);
        if (installerTypeItr != m_installationMetadata.end())
        {
            auto installerType = Manifest::ManifestInstaller::ConvertToInstallerTypeEnum(installerTypeItr->second);
            if (installer1.InstallerType == installerType && installer2.InstallerType != installerType)
            {
                return true;
            }
            if (Manifest::ManifestInstaller::IsInstallerTypeCompatible(installer1.InstallerType, installerType) &&
                !Manifest::ManifestInstaller::IsInstallerTypeCompatible(installer2.InstallerType, installerType))
            {
                return true;
            }
        }

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
        std::sort(installers.begin(), installers.end(), m_installerComparator);

        // If the first one's architecture is inapplicable, then no installer is applicable.
        if (Utility::IsApplicableArchitecture(installers[0].Arch) == Utility::InapplicableArchitecture)
        {
            return {};
        }

        // If the first one's InstallerType is inapplicable, then no installer is applicable.
        auto installerTypeItr = m_installationMetadata.find(s_InstallationMetadata_Key_InstallerType);
        if (installerTypeItr != m_installationMetadata.end())
        {
            auto installerType = Manifest::ManifestInstaller::ConvertToInstallerTypeEnum(installerTypeItr->second);
            if (!Manifest::ManifestInstaller::IsInstallerTypeCompatible(installers[0].InstallerType, installerType))
            {
                return {};
            }
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
            std::sort(localization.begin(), localization.end(), m_localizationComparator);

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