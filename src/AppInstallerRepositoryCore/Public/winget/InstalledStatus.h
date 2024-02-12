// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerStrings.h>
#include <winget/Manifest.h>
#include <winget/RepositorySearch.h>

#include <memory>
#include <vector>


namespace AppInstaller::Repository
{
    // Defines the installed status check type.
    enum class InstalledStatusType : uint32_t
    {
        // None is checked.
        None = 0x0,
        // Check Apps and Features entry.
        AppsAndFeaturesEntry = 0x0001,
        // Check Apps and Features entry install location if applicable.
        AppsAndFeaturesEntryInstallLocation = 0x0002,
        // Check Apps and Features entry install location with installed files if applicable.
        AppsAndFeaturesEntryInstallLocationFile = 0x0004,
        // Check default install location if applicable.
        DefaultInstallLocation = 0x0008,
        // Check default install location with installed files if applicable.
        DefaultInstallLocationFile = 0x0010,

        // Below are helper values for calling CheckInstalledStatus as input.
        // AppsAndFeaturesEntry related checks
        AllAppsAndFeaturesEntryChecks = AppsAndFeaturesEntry | AppsAndFeaturesEntryInstallLocation | AppsAndFeaturesEntryInstallLocationFile,
        // DefaultInstallLocation related checks
        AllDefaultInstallLocationChecks = DefaultInstallLocation | DefaultInstallLocationFile,
        // All checks
        AllChecks = AllAppsAndFeaturesEntryChecks | AllDefaultInstallLocationChecks,
    };

    DEFINE_ENUM_FLAG_OPERATORS(InstalledStatusType);

    // Struct representing an individual installed status.
    struct InstalledStatus
    {
        // The installed status type.
        InstalledStatusType Type = InstalledStatusType::None;
        // The installed status path.
        Utility::NormalizedString Path;
        // The installed status result.
        HRESULT Status;

        InstalledStatus(InstalledStatusType type, Utility::NormalizedString path, HRESULT status) :
            Type(type), Path(std::move(path)), Status(status) {}
    };

    // Struct representing installed status from an installer.
    struct InstallerInstalledStatus
    {
        Manifest::ManifestInstaller Installer;
        std::vector<InstalledStatus> Status;
    };

    // Checks installed status of a package.
    std::vector<InstallerInstalledStatus> CheckPackageInstalledStatus(const std::shared_ptr<ICompositePackage>& package, InstalledStatusType checkTypes = InstalledStatusType::AllChecks);
}
