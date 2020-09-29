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
        if (Utility::IsApplicableArchitecture(installer1.Arch) != -1 && Utility::IsApplicableArchitecture(installer2.Arch) == -1)
        {
            return true;
        }

        // If there's installation metadata, pick the preferred one or compatible one
        if (m_installationMetadata.has_value())
        {
            auto installedType = Manifest::ManifestInstaller::ConvertToInstallerTypeEnum(
                m_installationMetadata.value().find(s_InstallationMetadata_Key_InstallerType)->second);
            if (installer1.InstallerType == installedType && installer2.InstallerType != installedType)
            {
                return true;
            }
            if (Manifest::ManifestInstaller::IsInstallerTypeCompatible(installer1.InstallerType, installedType) &&
                !Manifest::ManifestInstaller::IsInstallerTypeCompatible(installer2.InstallerType, installedType))
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
        if (Utility::IsApplicableArchitecture(installers[0].Arch) == -1)
        {
            return {};
        }

        // If the first one's InstallerType is inapplicable, then no installer is applicable.
        if (m_installationMetadata.has_value())
        {
            std::string installedType = m_installationMetadata.value().find(s_InstallationMetadata_Key_InstallerType)->second;
            if (!Manifest::ManifestInstaller::IsInstallerTypeCompatible(
                installers[0].InstallerType,
                Manifest::ManifestInstaller::ConvertToInstallerTypeEnum(installedType)))
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