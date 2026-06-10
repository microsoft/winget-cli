// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "PackageDependenciesValidation.h"
#include <winget/ManifestValidation.h>
#include <winget/DependenciesGraph.h>

namespace AppInstaller::Repository
{
    namespace
    {
        struct DependentManifestInfo
        {
            Utility::NormalizedString Id;
            Utility::NormalizedString Version;
        };

        Manifest::DependencyList GetDependencies(
            const Manifest::Manifest& manifest, AppInstaller::Manifest::DependencyType dependencyType)
        {
            Manifest::DependencyList depList;
            std::vector<AppInstaller::Manifest::Dependency>  dependencies;

            for (const auto& installer : manifest.Installers)
            {
                installer.Dependencies.ApplyToType(dependencyType, [&](AppInstaller::Manifest::Dependency dependency)
                {
                    depList.Add(dependency);
                });
            }

            return depList;
        }

        std::optional<SQLiteIndex::VersionKey> GetPackageLatestVersion(
            SQLiteIndex* index, Manifest::string_t packageId, std::set<Utility::Version> exclusions = {})
        {
            SearchRequest request;
            request.Filters.emplace_back(PackageMatchField::Id, MatchType::CaseInsensitive, packageId);

            auto results = index->Search(request);

            if (results.Matches.empty())
            {
                return {};
            }

            auto packageRowId = results.Matches[0].first;
            auto vac = index->GetVersionKeysById(packageRowId);

            if (vac.empty())
            {
                return {};
            }

            SQLiteIndex::VersionKey maxVersion{ Utility::VersionAndChannel{ Utility::Version::CreateUnknown(), Utility::Channel("") } };

            for (auto& v : vac)
            {
                auto currentVersion = v.VersionAndChannel.GetVersion();
                if (exclusions.find(currentVersion) != exclusions.end())
                {
                    continue;
                }

                if (currentVersion > maxVersion.VersionAndChannel.GetVersion())
                {
                    maxVersion = v;
                }
            }

            if (maxVersion.VersionAndChannel.GetVersion().IsUnknown())
            {
                return {};
            }

            return maxVersion;
        }
    
        void ThrowOnManifestValidationFailed(
            std::vector<std::pair<DependentManifestInfo, Utility::Version>> failedManifests, AppInstaller::StringResource::StringId error)
        {
            auto itrStart = failedManifests.begin();
            std::vector<Manifest::ValidationError> validationErrors;
            validationErrors.emplace_back(error, "PackageIdentifier.PackageVersion", itrStart->first.Id + '.' + itrStart->first.Version );

            std::for_each(
                itrStart + 1,
                failedManifests.end(),
                [&](std::pair<DependentManifestInfo, Utility::Version> current)
                {
                    validationErrors.emplace_back(error, "PackageIdentifier.PackageVersion", current.first.Id + '.' + current.first.Version);
                });

            THROW_EXCEPTION(
                Manifest::ManifestException(
                    std::move(validationErrors), APPINSTALLER_CLI_ERROR_DEPENDENCIES_VALIDATION_FAILED));
        }
    };

    bool PackageDependenciesValidation::ValidateManifestDependencies(SQLiteIndex* index, const Manifest::Manifest& manifest)
    {
        using namespace Manifest;

        Dependency rootId(DependencyType::Package, manifest.Id, manifest.Version);
        std::vector<ValidationError> dependenciesError;
        bool foundErrors = false;

        DependencyGraph graph
        {
            rootId,
            [&](const Dependency& node)
            {
                DependencyList depList;
                if (node.Id() == rootId.Id())
                {
                    return GetDependencies(manifest, DependencyType::Package);
                }

                auto packageLatest = GetPackageLatestVersion(index, node.Id());
                if (!packageLatest.has_value())
                {
                    dependenciesError.emplace_back(
                        ManifestError::MissingManifestDependenciesNode, "PackageIdentifier", node.Id());
                    foundErrors = true;
                    return depList;
                }

                if (node.MinVersion > packageLatest->VersionAndChannel.GetVersion())
                {
                    dependenciesError.emplace_back(ManifestError::NoSuitableMinVersionDependency, "PackageIdentifier", node.Id());
                    foundErrors = true;
                    return depList;
                }

                auto packageLatestDependencies = index->GetDependenciesByManifestRowId(packageLatest->ManifestId);
                std::for_each(
                    packageLatestDependencies.begin(),
                    packageLatestDependencies.end(),
                    [&](std::pair<SQLite::rowid_t, Utility::NormalizedString> row)
                    {
                        auto manifestRowId = index->GetManifestIdByKey(row.first, "", "");
                        auto packageId = index->GetPropertyByPrimaryId(manifestRowId.value(), PackageVersionProperty::Id);
                        Dependency dep(DependencyType::Package, packageId.value(), row.second);
                        depList.Add(dep);
                    });

                return depList;
            }
        };

        graph.BuildGraph();

        if (foundErrors)
        {
            THROW_EXCEPTION(ManifestException(std::move(dependenciesError), APPINSTALLER_CLI_ERROR_DEPENDENCIES_VALIDATION_FAILED));
        }

        if (graph.HasLoop())
        {
            dependenciesError.emplace_back(ManifestError::FoundDependencyLoop);
            THROW_EXCEPTION(ManifestException(std::move(dependenciesError), APPINSTALLER_CLI_ERROR_DEPENDENCIES_VALIDATION_FAILED));
        }

        return true;
    }

    bool PackageDependenciesValidation::VerifyDependenciesStructureForManifestDelete(SQLiteIndex* index, const Manifest::Manifest& manifest)
    {
        auto dependentsSet = index->GetDependentsById(manifest.Id);

        if (!dependentsSet.size())
        {
            // all good this manifest is not a dependency of any manifest.
            return true;
        }

        std::vector<std::pair<DependentManifestInfo, Utility::Version>> dependentManifestInfoToVersionPair;
        std::for_each(
            dependentsSet.begin(),
            dependentsSet.end(),
            [&](std::pair<SQLite::rowid_t, Utility::Version> current)
            {
                DependentManifestInfo dependentManifestInfo;
                dependentManifestInfo.Id = index->GetPropertyByPrimaryId(current.first, PackageVersionProperty::Id).value();
                dependentManifestInfo.Version = index->GetPropertyByPrimaryId(current.first, PackageVersionProperty::Version).value();

                dependentManifestInfoToVersionPair.emplace_back(std::make_pair(dependentManifestInfo, current.second));
            });

        auto packageLatest = GetPackageLatestVersion(index, manifest.Id);

        if (!packageLatest.has_value())
        {
            // this is a fatal error, a manifest should exists in the very least(including the current manifest being deleted),
            // since this is a delete operation. 
            THROW_HR(APPINSTALLER_CLI_ERROR_MISSING_PACKAGE);
        }

        if (Utility::Version(manifest.Version) < packageLatest->VersionAndChannel.GetVersion())
        {
            // all good, since it's min version the criteria is still satisfied.
            return true;
        }

        auto nextLatestAfterDelete = GetPackageLatestVersion(index, manifest.Id, { packageLatest->VersionAndChannel.GetVersion() });

        if (!nextLatestAfterDelete.has_value())
        {
            ThrowOnManifestValidationFailed(
                dependentManifestInfoToVersionPair, Manifest::ManifestError::SingleManifestPackageHasDependencies);
        }

        std::vector<std::pair<DependentManifestInfo, Utility::Version>> breakingManifests;

        // Gets breaking manifests.
        std::copy_if(
            dependentManifestInfoToVersionPair.begin(),
            dependentManifestInfoToVersionPair.end(),
            std::back_inserter(breakingManifests),
            [&](std::pair<DependentManifestInfo, Utility::Version> current)
            {
                return current.second > nextLatestAfterDelete->VersionAndChannel.GetVersion();
            }
        );

        if (!breakingManifests.empty())
        {
            ThrowOnManifestValidationFailed(
                breakingManifests, Manifest::ManifestError::MultiManifestPackageHasDependencies);
        }

        return true;
    }
}
