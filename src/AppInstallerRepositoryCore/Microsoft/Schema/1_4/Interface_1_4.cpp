// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_4/Interface.h"
#include "Microsoft/Schema/1_0/VersionTable.h"

#include "Microsoft/Schema/1_4/DependenciesTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::V1_4
{
    Interface::Interface(Utility::NormalizationVersion normVersion) : V1_3::Interface(normVersion)
    {
    }

    SQLite::Version Interface::GetVersion() const
    {
        return { 1, 4 };
    }

    void Interface::CreateTables(SQLite::Connection& connection, CreateOptions options)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createtables_v1_4");

        V1_3::Interface::CreateTables(connection, options);

        if (WI_IsFlagClear(options, CreateOptions::DisableDependenciesSupport))
        {
            DependenciesTable::Create(connection);
        }

        savepoint.Commit();
    }

    SQLite::rowid_t Interface::AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addmanifest_v1_4");

        SQLite::rowid_t manifestId = V1_3::Interface::AddManifest(connection, manifest, relativePath);

        DependenciesTable::AddDependencies(connection, manifest, manifestId);

        savepoint.Commit();

        return manifestId;
    }

    std::pair<bool, SQLite::rowid_t> Interface::UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "updatemanifest_v1_4");

        auto [indexModified, manifestId] = V1_3::Interface::UpdateManifest(connection, manifest, relativePath);

        bool dependenciesModified = DependenciesTable::UpdateDependencies(connection, manifest, manifestId);
        indexModified = indexModified || dependenciesModified;

        savepoint.Commit();

        return { indexModified, manifestId };
     }

    void Interface::RemoveManifestById(SQLite::Connection& connection, SQLite::rowid_t manifestId)
    {
        // Get all versions that need cleaning from the version table.
        auto minVersions = DependenciesTable::GetDependenciesMinVersionsRowIdByManifestId(connection, manifestId);

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "removemanifest_v1_4");

        // Removes dependences for the manifest id.
        DependenciesTable::RemoveDependencies(connection, manifestId);

        // Removes the manifest.
        V1_3::Interface::RemoveManifestById(connection, manifestId);

        // Remove the versions that are not needed.
        for (auto minVersion : minVersions)
        {
            if (NotNeeded(connection, Schema::V1_0::VersionTable::TableName(), Schema::V1_0::VersionTable::ValueName(), minVersion))
            {
                Schema::V1_0::VersionTable::DeleteById(connection, minVersion);
            }
        }

        savepoint.Commit();
    }

    bool Interface::NotNeeded(const SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, SQLite::rowid_t id) const
    {
        bool result = V1_0::Interface::NotNeeded(connection, tableName, valueName, id);

        return !DependenciesTable::IsValueReferenced(connection, tableName, id) && result;
    }

    void Interface::PrepareForPackaging(SQLite::Connection& connection, bool vacuum)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "prepareforpackaging_v1_4");

        V1_3::Interface::PrepareForPackaging(connection, false);

        DependenciesTable::PrepareForPackaging(connection);

        savepoint.Commit();

        if (vacuum)
        {
            Vacuum(connection);
        }
    }

    bool Interface::CheckConsistency(const SQLite::Connection& connection, bool log) const
    {
        bool result = V1_3::Interface::CheckConsistency(connection, log);

        // If the v1.3 index was consistent, or if full logging of inconsistency was requested, check the v1.4 data.
        if (result || log)
        {
            result = DependenciesTable::CheckConsistency(connection, log) && result;
        }

        if (result || log)
        {
            result = ValidateDependenciesWithMinVersions(connection, log) && result;
        }

        return result;
    }

    std::set<std::pair<SQLite::rowid_t, Utility::NormalizedString>> Interface::GetDependenciesByManifestRowId(const SQLite::Connection& connection, SQLite::rowid_t manifestRowId) const
    {
        return DependenciesTable::GetDependenciesByManifestRowId(connection, manifestRowId);
    }

    std::vector<std::pair<SQLite::rowid_t, Utility::NormalizedString>> Interface::GetDependentsById(const SQLite::Connection& connection, AppInstaller::Manifest::string_t packageId) const
    {
        return DependenciesTable::GetDependentsById(connection, packageId);
    }

    void Interface::DropTables(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "drop_tables_v1_4");

        V1_2::Interface::DropTables(connection);

        DependenciesTable::Drop(connection);

        savepoint.Commit();
    }

    bool Interface::ValidateDependenciesWithMinVersions(const SQLite::Connection& connection, bool log) const
    {
        try
        {
            bool result = true;
            // A map to store already checked dependency package latest versions.
            std::map<SQLite::rowid_t, Utility::Version> checkedVersions;

            auto dependencies = DependenciesTable::GetAllDependenciesWithMinVersions(connection);
            for (auto const& dependency : dependencies)
            {
                // If the dependency package has not been checked yet, add to the map.
                if (checkedVersions.find(dependency.first) == checkedVersions.end())
                {
                    auto versionKeys = GetVersionKeysById(connection, dependency.first);
                    THROW_HR_IF(E_UNEXPECTED, versionKeys.empty());
                    checkedVersions.emplace(dependency.first, versionKeys[0].VersionAndChannel.GetVersion());
                }

                // If the latest version is less than min version required, fail the validation.
                if (checkedVersions[dependency.first] < Utility::Version{ dependency.second })
                {
                    AICLI_LOG(Repo, Error, << "Dependency with min version not satisfied. Dependency package row id: " << dependency.first << " min version: " << dependency.second);
                    result = false;

                    if (!log)
                    {
                        break;
                    }
                }
            }

            return result;
        }
        catch (...)
        {
            AICLI_LOG(Repo, Error, << "ValidateDependenciesWithMinVersions() encountered internal error. Returning false.");
            return false;
        }
    }
}
