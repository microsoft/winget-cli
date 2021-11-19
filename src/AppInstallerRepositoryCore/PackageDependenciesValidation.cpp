// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include <AppInstallerVersions.h>
#include <winget/Manifest.h>
#include <winget/ManifestValidation.h>
#include <SQLiteWrapper.h>
#include <PackageDependenciesValidation.h>
#include <Microsoft/Schema/1_4/DependenciesTable.h>
#include "Microsoft/Schema/1_0/ManifestTable.h"
#include <winget/DependenciesGraph.h>

namespace AppInstaller::Repository
{
	using namespace Microsoft::Schema::V1_4;
	using namespace Microsoft::Schema::V1_0;

	namespace
	{
		std::vector<AppInstaller::Manifest::Dependency> GetDependencies(
			const Manifest::Manifest& manifest, AppInstaller::Manifest::DependencyType dependencyType)
		{
			std::vector<AppInstaller::Manifest::Dependency>  dependencies;

			for (const auto& installer : manifest.Installers)
			{
				installer.Dependencies.ApplyToAll([&](AppInstaller::Manifest::Dependency dependency)
					{
						if (dependency.Type == dependencyType)
						{
							dependencies.emplace_back(dependency);
						}
					});
			}

			return dependencies;
		}

		std::optional<std::pair<SQLite::rowid_t, Utility::Version>> GetPackageLatestVersion(
			SQLiteIndex* index, Manifest::string_t packageId, std::set<Utility::Version> exclusions = {})
		{
			SearchRequest request;
			request.Filters.emplace_back(PackageMatchField::Id, MatchType::Exact, packageId);
			std::optional<std::pair<SQLite::rowid_t, Utility::Version>> result;

			auto results = index->Search(request);

			if (results.Matches.empty())
			{
				return result;
			}

			auto packageRowId = results.Matches[0].first;

			auto vac = index->GetVersionKeysById(packageRowId);

			Utility::Version maxVersion = Utility::Version::CreateUnknown();

			for (auto& v : vac)
			{
				auto currentVersion = v.GetVersion();
				if (exclusions.find(currentVersion) != exclusions.end())
				{
					continue;
				}

				if (maxVersion.IsUnknown() || currentVersion > maxVersion)
				{
					maxVersion = currentVersion;
				}
			}

			if (!maxVersion.IsUnknown())
			{
				auto manifestRowId = index->GetManifestIdByKey(packageRowId, maxVersion.ToString(), "");
				result = std::make_pair(manifestRowId.value(), maxVersion);
			}

			return result;
		}
	};

	bool PackageDependenciesValidation::ValidateManifestDependencies(SQLiteIndex* index, const Manifest::Manifest manifest)
	{
		using namespace Manifest;

		Dependency rootId(DependencyType::Package, manifest.Id, manifest.Version);
		std::vector<ValidationError> dependenciesError;
		bool foundErrors = false;

		DependencyGraph graph(rootId, [&](const Dependency& node) {

			DependencyList depList;
			if (node.Id == rootId.Id)
			{
				const auto dependencies = GetDependencies(manifest, DependencyType::Package);
				if (!dependencies.size())
				{
					return depList;
				}

				std::for_each(dependencies.begin(), dependencies.end(), [&](Dependency dep) { depList.Add(dep);  });
				return depList;
			}

			auto packageLatest = GetPackageLatestVersion(index, node.Id);
			if (!packageLatest.has_value())
			{
				std::string error = ManifestError::MissingManifestDependenciesNode;
				error.append(" ").append(node.Id);
				dependenciesError.emplace_back(ValidationError(error));
				foundErrors = true;
				return depList;
			}

			if (node.MinVersion > packageLatest.value().second)
			{
				std::string error = ManifestError::NoSuitableMinVersion;
				error.append(" ").append(node.Id);
				dependenciesError.emplace_back(ValidationError(error));
				foundErrors = true;
				return depList;
			}

			auto packageLatestDependencies = index->GetDependenciesByManifestRowId(packageLatest.value().first);
			std::for_each(
				packageLatestDependencies.begin(),
				packageLatestDependencies.end(),
				[&](std::pair<Dependency, SQLite::rowid_t> row)
				{
					depList.Add(row.first);
				});

			return depList;
			});

		graph.BuildGraph();

		if (foundErrors)
		{
			THROW_EXCEPTION(ManifestException(std::move(dependenciesError), APPINSTALLER_CLI_ERROR_DEPENDENCIES_VALIDATION_FAILED));
		}

		if (graph.HasLoop())
		{
			std::string error = ManifestError::FoundLoop;
			dependenciesError.emplace_back(error);
			THROW_EXCEPTION(ManifestException(std::move(dependenciesError), APPINSTALLER_CLI_ERROR_DEPENDENCIES_VALIDATION_FAILED));
		}

		return true;
	}

	bool PackageDependenciesValidation::VerifyDependenciesStructureForManifestDelete(SQLiteIndex* index, const Manifest::Manifest manifest)
	{
		auto dependentsSet = index->GetDependentsById(manifest.Id);
		
		if (!dependentsSet.size())
		{
			// all good this manifest is not a dependency of any manifest.
			return true;
		}

		std::vector<std::pair<Manifest::Manifest, Utility::Version>> manifestToVersionPair;
		std::for_each(
			dependentsSet.begin(),
			dependentsSet.end(),
			[&](std::pair<SQLite::rowid_t, Utility::Version> current)
			{
				Manifest::Manifest manifest;
				manifest.Id = index->GetPropertyByManifestId(current.first, PackageVersionProperty::Id).value();
				manifest.Version = index->GetPropertyByManifestId(current.first, PackageVersionProperty::Version).value();

				manifestToVersionPair.emplace_back(std::make_pair(manifest, current.second));
			});

		auto packageLatest = GetPackageLatestVersion(index, manifest.Id);

		if (!packageLatest.has_value())
		{
			// this is a fatal error, a manifest should exists in the very least(including the current manifest being deleted),
			// since this is a delete operation. 
			THROW_HR(APPINSTALLER_CLI_ERROR_MISSING_PACKAGE);
		}

		if (Utility::Version(manifest.Version) < packageLatest.value().second)
		{
			// all good, since it's min version the criteria is still satisfied.
			return true;
		}

		auto nextLatestAfterDelete = GetPackageLatestVersion(index, manifest.Id, { packageLatest.value().second });

		if (!nextLatestAfterDelete.has_value())
		{
			auto itrStart = manifestToVersionPair.begin();
			std::string dependentPackages { itrStart->first.Id + "." + itrStart->first.Version };

			std::for_each(
				itrStart + 1,
				manifestToVersionPair.end(),
				[&](std::pair<Manifest::Manifest, Utility::Version> current)
				{
					dependentPackages.append(", " + current.first.Id + "." + current.first.Version);
				});

			std::string error = Manifest::ManifestError::SingleManifestPackageHasDependencies;
			error.append("\n" + dependentPackages);
			THROW_EXCEPTION(
				Manifest::ManifestException({ Manifest::ValidationError(error) },
					APPINSTALLER_CLI_ERROR_DEPENDENCIES_VALIDATION_FAILED));

		}

		std::vector<std::pair<Manifest::Manifest, Utility::Version>> breakingManifests;

		std::copy_if(
			manifestToVersionPair.begin(),
			manifestToVersionPair.end(),
			std::back_inserter(breakingManifests),
			[&](std::pair<Manifest::Manifest, Utility::Version> current)
			{
				return current.second > nextLatestAfterDelete.value().second;
			}
		);

		if (breakingManifests.size())
		{
			auto itrStart = breakingManifests.begin();
			std::string dependentPackages{ itrStart->first.Id + "." + itrStart->first.Version};

			std::for_each(
				itrStart + 1,
				breakingManifests.end(),
				[&](std::pair<Manifest::Manifest, Utility::Version> current)
				{
					dependentPackages.append(", " + current.first.Id + "." + current.first.Version);
				});

			std::string error = Manifest::ManifestError::MultiManifestPackageHasDependencies;
			error.append("\n" + dependentPackages);
			THROW_EXCEPTION(
				Manifest::ManifestException({ Manifest::ValidationError(error) },
					APPINSTALLER_CLI_ERROR_DEPENDENCIES_VALIDATION_FAILED));
		}

		return true;
	}
}