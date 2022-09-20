// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSource.h"
#include <public/winget/PackageDependenciesValidationUtil.h>

using namespace AppInstaller::Manifest;

TEST_CASE("GetValidationResultFromException", "[PackageDependenciesValidationUtil][dependencies]")
{
    std::map<WinGetManifestDependenciesErrorResult, const ManifestException> dependenciesErrorMessageMap =
    {
        { WinGetManifestDependenciesErrorResult::SingleManifestPackageHasDependencies, ManifestException {  { ManifestError::SingleManifestPackageHasDependencies }, APPINSTALLER_CLI_ERROR_MANIFEST_FAILED } },
        { WinGetManifestDependenciesErrorResult::MultiManifestPackageHasDependencies, ManifestException {  { ManifestError::MultiManifestPackageHasDependencies }, APPINSTALLER_CLI_ERROR_MANIFEST_FAILED } },
        { WinGetManifestDependenciesErrorResult::MissingManifestDependenciesNode, ManifestException {  { ManifestError::MissingManifestDependenciesNode }, APPINSTALLER_CLI_ERROR_MANIFEST_FAILED } },
        { WinGetManifestDependenciesErrorResult::NoSuitableMinVersionDependency, ManifestException {  { ManifestError::NoSuitableMinVersionDependency }, APPINSTALLER_CLI_ERROR_MANIFEST_FAILED } },
        { WinGetManifestDependenciesErrorResult::FoundDependencyLoop, ManifestException {  { ManifestError::FoundDependencyLoop }, APPINSTALLER_CLI_ERROR_MANIFEST_FAILED } },
    };

    for (auto current : dependenciesErrorMessageMap)
    {
        REQUIRE(GetDependenciesValidationResultFromException(current.second) == current.first);
    }
}