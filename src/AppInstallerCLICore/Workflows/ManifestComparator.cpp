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
        // TODO: Implement a mechanism for better error messaging for no applicable installer scenario
        bool IsInstallerApplicable(const Manifest::ManifestInstaller& installer, Manifest::InstallerTypeEnum installedType)
        {
            // Check MinOSVersion
            if (!installer.MinOSVersion.empty() &&
                !Runtime::IsCurrentOSVersionGreaterThanOrEqual(Utility::Version(installer.MinOSVersion)))
            {
                return false;
            }

            if (Utility::IsApplicableArchitecture(installer.Arch) == Utility::InapplicableArchitecture)
            {
                return false;
            }

            if (installedType != Manifest::InstallerTypeEnum::Unknown &&
                !Manifest::IsInstallerTypeCompatible(installer.InstallerType, installedType))
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
            Manifest::InstallerTypeEnum installedType)
        {
            // If there's installation metadata, pick the preferred one or compatible one
            if (installedType != Manifest::InstallerTypeEnum::Unknown)
            {
                if (installer1.InstallerType == installedType && installer2.InstallerType != installedType)
                {
                    return true;
                }
            }

            // Todo: Compare only architecture for now. Need more work and spec.
            auto arch1 = Utility::IsApplicableArchitecture(installer1.Arch);
            auto arch2 = Utility::IsApplicableArchitecture(installer2.Arch);

            if (arch1 > arch2)
            {
                return true;
            }

            return false;
        }
    }

    std::optional<Manifest::ManifestInstaller> ManifestComparator::GetPreferredInstaller(const Manifest::Manifest& manifest)
    {
        AICLI_LOG(CLI, Info, << "Starting installer selection.");

        // Get the currently installed package's type (if present)
        Manifest::InstallerTypeEnum installedType = Manifest::InstallerTypeEnum::Unknown;
        auto installerTypeItr = m_installationMetadata.find(Repository::PackageVersionMetadata::InstalledType);
        if (installerTypeItr != m_installationMetadata.end())
        {
            installedType = Manifest::ConvertToInstallerTypeEnum(installerTypeItr->second);
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
            else if (IsInstallerApplicable(installer, installedType) && IsInstallerBetterMatch(installer, *result, installedType))
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
            Manifest::InstallerTypeToString(result->InstallerType),
            Manifest::ScopeToString(result->Scope),
            result->Locale);

        return *result;
    }
}