// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "PackageDependenciesValidationUtil.h"
#include <winget/ManifestValidation.h>

namespace AppInstaller::Repository::Util
{
    using namespace AppInstaller::Manifest;

    WinGetManifestDependenciesErrorResult GetValidationResultFromException(const AppInstaller::Manifest::ManifestException& manifestException)
	{
        auto validationErrors = manifestException.Errors();
        for (auto validationError : validationErrors)
        {
            auto message = validationError.Message;

            std::map<AppInstaller::StringResource::StringId, WinGetManifestDependenciesErrorResult> dependenciesErrorMessageMap =
            {
                {ManifestError::SingleManifestPackageHasDependencies, WinGetManifestDependenciesErrorResult::SingleManifestPackageHasDependencies},
                {ManifestError::MultiManifestPackageHasDependencies, WinGetManifestDependenciesErrorResult::MultiManifestPackageHasDependencies },
                {ManifestError::MissingManifestDependenciesNode, WinGetManifestDependenciesErrorResult::MissingManifestDependenciesNode },
                {ManifestError::NoSuitableMinVersionDependency, WinGetManifestDependenciesErrorResult::NoSuitableMinVersionDependency },
                {ManifestError::FoundDependencyLoop, WinGetManifestDependenciesErrorResult::FoundDependencyLoop}
            };

            auto itr = dependenciesErrorMessageMap.find(message);

            if (itr != dependenciesErrorMessageMap.end())
            {
                return itr->second;
            }
        }

        return WinGetManifestDependenciesErrorResult::None;
	}
}