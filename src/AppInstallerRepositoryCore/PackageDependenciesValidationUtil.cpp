// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "PackageDependenciesValidationUtil.h"
#include <winget/ManifestValidation.h>

namespace AppInstaller::Repository::Util
{
    WinGetManifestDependenciesErrorResult GetValidationResultFromException(const AppInstaller::Manifest::ManifestException& manifestException)
	{
        auto validationErrors = manifestException.Errors();
        for (auto validationError : validationErrors)
        {
            auto message = validationError.Message;

            std::map<AppInstaller::StringResource::StringId, WinGetManifestDependenciesErrorResult> dependenciesErrorMessageMap =
            {
                {AppInstaller::Manifest::ManifestError::SingleManifestPackageHasDependencies, WinGetManifestDependenciesErrorResult::SingleManifestPackageHasDependencies},
                {AppInstaller::Manifest::ManifestError::MultiManifestPackageHasDependencies, WinGetManifestDependenciesErrorResult::MultiManifestPackageHasDependencies },
                {AppInstaller::Manifest::ManifestError::MissingManifestDependenciesNode, WinGetManifestDependenciesErrorResult::MissingManifestDependenciesNode },
                {AppInstaller::Manifest::ManifestError::NoSuitableMinVersionDependency, WinGetManifestDependenciesErrorResult::NoSuitableMinVersionDependency },
                {AppInstaller::Manifest::ManifestError::FoundDependencyLoop, WinGetManifestDependenciesErrorResult::FoundDependencyLoop}
            };

            if (dependenciesErrorMessageMap.find(message) != dependenciesErrorMessageMap.end())
            {
                return dependenciesErrorMessageMap[message];
            }
        }

        return WinGetManifestDependenciesErrorResult::None;
	}
}