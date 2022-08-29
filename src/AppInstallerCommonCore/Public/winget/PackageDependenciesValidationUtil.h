// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/ManifestValidation.h>

namespace AppInstaller::Manifest
{
    enum WinGetManifestDependenciesErrorResult
    {
        None = 0x0,

        // Each validation step should have an enum for corresponding failure.
        SingleManifestPackageHasDependencies = 0x10000,
        MultiManifestPackageHasDependencies = 0x20000,
        MissingManifestDependenciesNode = 0x40000,
        NoSuitableMinVersionDependency = 0x80000,
        FoundDependencyLoop = 0x100000,
    };

    DEFINE_ENUM_FLAG_OPERATORS(WinGetManifestDependenciesErrorResult);

    WinGetManifestDependenciesErrorResult GetDependenciesValidationResultFromException(const ManifestException& manifestException);
}