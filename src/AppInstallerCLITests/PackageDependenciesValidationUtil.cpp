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
        {WinGetManifestDependenciesErrorResult::NoSuitableMinVersionDependency, AppInstaller::Manifest::ManifestException {  { AppInstaller::Manifest::ManifestError::NoSuitableMinVersionDependency }, APPINSTALLER_CLI_ERROR_MANIFEST_FAILED } },
        {WinGetManifestDependenciesErrorResult::FoundDependencyLoop, AppInstaller::Manifest::ManifestException {  { AppInstaller::Manifest::ManifestError::FoundDependencyLoop }, APPINSTALLER_CLI_ERROR_MANIFEST_FAILED } },
    };

    for (auto current : dependenciesErrorMessageMap)
    {
        REQUIRE(GetDependenciesValidationResultFromException(current.second) == current.first);
    }
}