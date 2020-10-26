// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowBase.h"
#include "ManifestComparator.h"

using namespace AppInstaller::CLI;
using namespace AppInstaller::Manifest;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        // Determine if the installer is applicable.
        bool IsInstallerApplicable(const Manifest::ManifestInstaller& installer, Manifest::ManifestInstaller::InstallerTypeEnum installedType)
        {
            if (Utility::IsApplicableArchitecture(installer.Arch) == Utility::InapplicableArchitecture)
            {
                return false;
            }

            if (installedType != Manifest::ManifestInstaller::InstallerTypeEnum::Unknown &&
                !Manifest::ManifestInstaller::IsInstallerTypeCompatible(installer.InstallerType, installedType))
            {
                return false;
            }

            return true;
        }

        // This is used in sorting the list of available installers to get the best match.
        // Determines if installer1 is a better match than installer2.
        bool IsInstallerBetterMatch(
            const Manifest::ManifestInstaller& installer1,
            const Manifest::ManifestInstaller& installer2,
            Manifest::ManifestInstaller::InstallerTypeEnum installedType)
        {
            auto arch1 = Utility::IsApplicableArchitecture(installer1.Arch);
            auto arch2 = Utility::IsApplicableArchitecture(installer2.Arch);

            // Applicable architecture should always come before inapplicable architecture
            if (arch1 != Utility::InapplicableArchitecture &&
                arch2 == Utility::InapplicableArchitecture)
            {
                return true;
            }

            // If there's installation metadata, pick the preferred one or compatible one
            if (installedType != Manifest::ManifestInstaller::InstallerTypeEnum::Unknown)
            {
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
            if (arch1 > arch2)
            {
                return true;
            }

            return false;
        }

        // This is used in sorting the list of available localizations to get the best match.
        struct LocalizationComparator
        {
            bool operator() (
                const Manifest::ManifestLocalization& loc1,
                const Manifest::ManifestLocalization& loc2)
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
        };
    }

    std::optional<Manifest::ManifestInstaller> ManifestComparator::GetPreferredInstaller(const Manifest::Manifest& manifest)
    {
        AICLI_LOG(CLI, Info, << "Starting installer selection.");

        // Get the currently installed package's type (if present)
        Manifest::ManifestInstaller::InstallerTypeEnum installedType = Manifest::ManifestInstaller::InstallerTypeEnum::Unknown;
        auto installerTypeItr = m_installationMetadata.find(Repository::PackageVersionMetadata::InstalledType);
        if (installerTypeItr != m_installationMetadata.end())
        {
            installedType = Manifest::ManifestInstaller::ConvertToInstallerTypeEnum(installerTypeItr->second);
        }

        const Manifest::ManifestInstaller* result = nullptr;

        for (const auto& installer : manifest.Installers)
        {
            if (!result)
            {
                if (IsInstallerApplicable(installer, installedType))
                {
                    result = &installer;
                }
            }
            else if (IsInstallerBetterMatch(installer, *result, installedType))
            {
                result = &installer;
            }
        }

        if (!result)
        {
            return {};
        }

        Logging::Telemetry().LogSelectedInstaller(
            static_cast<int>(result->Arch),
            result->Url,
            Manifest::ManifestInstaller::InstallerTypeToString(result->InstallerType),
            Manifest::ManifestInstaller::ScopeToString(result->Scope),
            result->Language);

        return *result;
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