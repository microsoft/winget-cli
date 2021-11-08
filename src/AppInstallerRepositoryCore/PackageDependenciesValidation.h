// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerVersions.h>
#include <winget/Manifest.h>
#include <SQLiteWrapper.h>

namespace AppInstaller::Repository
{
    struct PackageDependenciesValidation
    {
        // Validate the dependencies of the given manifest.
        static bool ValidateManifestDependencies(const std::string& indexPath, const Manifest::Manifest& manifest);

        static bool VerifyDependenciesStructureForManifestDelete(const std::string& indexPath, const Manifest::Manifest&);
    };
}
