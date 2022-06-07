// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/SQLiteIndex.h"
#include <winget/Manifest.h>

namespace AppInstaller::Repository
{
    using namespace AppInstaller::Repository::Microsoft;

    struct PackageDependenciesValidation
    {
        // Validate the dependencies of the given manifest.
        static bool ValidateManifestDependencies(SQLiteIndex* index, const Manifest::Manifest& manifest);

        static bool VerifyDependenciesStructureForManifestDelete(SQLiteIndex* index, const Manifest::Manifest& manifest);
    };
}
