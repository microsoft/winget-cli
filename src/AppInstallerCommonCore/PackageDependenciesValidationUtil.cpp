// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <Public/winget/PackageDependenciesValidationUtil.h>
#include <winget/ManifestValidation.h>

namespace AppInstaller::Manifest
{
	WinGetManifestDependenciesErrorResult GetDependenciesValidationResultFromException(const AppInstaller::Manifest::ManifestException& manifestException)
	{
		auto result = WinGetManifestDependenciesErrorResult::None;
		auto validationErrors = manifestException.Errors();

		for (const auto& validationError : validationErrors)
		{
			auto message = validationError.Message;

			std::map<AppInstaller::StringResource::StringId, WinGetManifestDependenciesErrorResult> dependenciesErrorMessageMap =
			{
				{ManifestError::SingleManifestPackageHasDependencies, WinGetManifestDependenciesErrorResult::SingleManifestPackageHasDependencies},
				{ManifestError::MultiManifestPackageHasDependencies, WinGetManifestDependenciesErrorResult::MultiManifestPackageHasDependencies },
				{ManifestError::MissingManifestDependenciesNode, WinGetManifestDependenciesErrorResult::MissingManifestDependenciesNode },
				{ManifestError::NoSuitableMinVersionDependency, WinGetManifestDependenciesErrorResult::NoSuitableMinVersionDependency },
				{ManifestError::FoundDependencyLoop, WinGetManifestDependenciesErrorResult::FoundDependencyLoop }
			};

			auto itr = dependenciesErrorMessageMap.find(message);

			if (itr != dependenciesErrorMessageMap.end())
			{
				result |= itr->second;
			}
		}

		return result;
	}
}