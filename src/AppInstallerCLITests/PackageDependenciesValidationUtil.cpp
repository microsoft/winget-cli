#include "pch.h"
#include "TestCommon.h"
#include "TestSource.h"
#include "PackageDependenciesValidationUtil.h"

using namespace AppInstaller::Repository::Util;

TEST_CASE("GetValidationResultFromException", "[PackageDependenciesValidationUtil][dependencies]")
{
    std::map<WinGetManifestDependenciesErrorResult, const AppInstaller::Manifest::ManifestException> dependenciesErrorMessageMap =
    {
        {WinGetManifestDependenciesErrorResult::SingleManifestPackageHasDependencies, AppInstaller::Manifest::ManifestException {  { AppInstaller::Manifest::ManifestError::SingleManifestPackageHasDependencies }, APPINSTALLER_CLI_ERROR_MANIFEST_FAILED }},
        {WinGetManifestDependenciesErrorResult::MultiManifestPackageHasDependencies, AppInstaller::Manifest::ManifestException {  { AppInstaller::Manifest::ManifestError::MultiManifestPackageHasDependencies }, APPINSTALLER_CLI_ERROR_MANIFEST_FAILED } },
        {WinGetManifestDependenciesErrorResult::MissingManifestDependenciesNode, AppInstaller::Manifest::ManifestException {  { AppInstaller::Manifest::ManifestError::MissingManifestDependenciesNode }, APPINSTALLER_CLI_ERROR_MANIFEST_FAILED } },
        {WinGetManifestDependenciesErrorResult::NoSuitableMinVersion, AppInstaller::Manifest::ManifestException {  { AppInstaller::Manifest::ManifestError::NoSuitableMinVersion }, APPINSTALLER_CLI_ERROR_MANIFEST_FAILED } },
        {WinGetManifestDependenciesErrorResult::FoundLoop, AppInstaller::Manifest::ManifestException {  { AppInstaller::Manifest::ManifestError::FoundLoop }, APPINSTALLER_CLI_ERROR_MANIFEST_FAILED } },
    };

    for (auto current : dependenciesErrorMessageMap)
    {
        REQUIRE(GetValidationResultFromException(current.second) == current.first);
    }
}