// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_6/Interface.h"
#include "Microsoft/Schema/1_6/UpgradeCodeTable.h"
#include "Microsoft/Schema/1_6/SearchResultsTable.h"
#include "Microsoft/Schema/1_0/ManifestTable.h"
#include "Microsoft/Schema/1_0/VersionTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::V1_6
{
    Interface::Interface(Utility::NormalizationVersion normVersion) : V1_5::Interface(normVersion)
    {
    }

    SQLite::Version Interface::GetVersion() const
    {
        return { 1, 6 };
    }

    void Interface::CreateTables(SQLite::Connection& connection, CreateOptions options)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createtables_v1_6");

        V1_5::Interface::CreateTables(connection, options);

        UpgradeCodeTable::Create(connection, GetOneToManyTableSchema());

        savepoint.Commit();
    }

    SQLite::rowid_t Interface::AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addmanifest_v1_6");

        SQLite::rowid_t manifestId = V1_5::Interface::AddManifest(connection, manifest, relativePath);

        // Add the new 1.6 data
        // These system reference strings are all stored with their cases folded so that they can be
        // looked up ordinally; enabling the index to provide efficient searches.
        UpgradeCodeTable::EnsureExistsAndInsert(connection, manifest.GetUpgradeCodes(), manifestId);

        savepoint.Commit();

        return manifestId;
    }

    std::pair<bool, SQLite::rowid_t> Interface::UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "updatemanifest_v1_6");

        auto [indexModified, manifestId] = V1_5::Interface::UpdateManifest(connection, manifest, relativePath);

        // Update new 1:N tables as necessary
        indexModified = UpgradeCodeTable::UpdateIfNeededByManifestId(connection, manifest.GetUpgradeCodes(), manifestId) || indexModified;

        savepoint.Commit();

        return { indexModified, manifestId };
    }

    void Interface::RemoveManifestById(SQLite::Connection& connection, SQLite::rowid_t manifestId)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "RemoveManifestById_v1_6");

        // Removes the manifest.
        V1_5::Interface::RemoveManifestById(connection, manifestId);

        // Remove all of the new 1:N data that is no longer referenced.
        UpgradeCodeTable::DeleteIfNotNeededByManifestId(connection, manifestId);

        savepoint.Commit();
    }

    bool Interface::CheckConsistency(const SQLite::Connection& connection, bool log) const
    {
        bool result = V1_5::Interface::CheckConsistency(connection, log);

        // If the v1.5 index was consistent, or if full logging of inconsistency was requested, check the v1.6 data.
        if (result || log)
        {
            result = UpgradeCodeTable::CheckConsistency(connection, log) && result;
        }

        return result;
    }

    std::vector<std::string> Interface::GetMultiPropertyByPrimaryId(const SQLite::Connection& connection, SQLite::rowid_t primaryId, PackageVersionMultiProperty property) const
    {
        switch (property)
        {
        case PackageVersionMultiProperty::UpgradeCode:
            return UpgradeCodeTable::GetValuesByManifestId(connection, primaryId);
        default:
            return V1_5::Interface::GetMultiPropertyByPrimaryId(connection, primaryId, property);
        }
    }

    void Interface::DropTables(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "drop_tables_v1_6");

        V1_4::Interface::DropTables(connection);

        UpgradeCodeTable::Drop(connection);

        savepoint.Commit();
    }

    std::unique_ptr<V1_0::SearchResultsTable> Interface::CreateSearchResultsTable(const SQLite::Connection& connection) const
    {
        return std::make_unique<V1_6::SearchResultsTable>(connection);
    }

    void Interface::PerformQuerySearch(V1_0::SearchResultsTable& resultsTable, const RequestMatch& query) const
    {
        // First, do an exact match search for the folded system reference strings
        // We do this first because it is exact, and likely won't match anything else if it matches this.
        PackageMatchFilter filter(PackageMatchField::UpgradeCode, MatchType::Exact, Utility::FoldCase(query.Value));
        resultsTable.SearchOnField(filter);

        // Then do the 1.5 search
        V1_5::Interface::PerformQuerySearch(resultsTable, query);
    }

    ISQLiteIndex::SearchResult Interface::SearchInternal(const SQLite::Connection& connection, SearchRequest& request) const
    {
        // Update any system reference strings to be folded
        auto foldIfNeeded = [](PackageMatchFilter& filter)
        {
            if (filter.Field == PackageMatchField::UpgradeCode && filter.Type == MatchType::Exact)
            {
                filter.Value = Utility::FoldCase(filter.Value);
            }
        };

        for (auto& inclusion : request.Inclusions)
        {
            foldIfNeeded(inclusion);
        }

        for (auto& filter : request.Filters)
        {
            foldIfNeeded(filter);
        }

        return V1_5::Interface::SearchInternal(connection, request);
    }

    void Interface::PrepareForPackaging(SQLite::Connection& connection, bool vacuum)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "prepareforpackaging_v1_6");

        V1_5::Interface::PrepareForPackaging(connection, false);

        UpgradeCodeTable::PrepareForPackaging(connection, GetOneToManyTableSchema(), true, true);

        savepoint.Commit();

        if (vacuum)
        {
            Vacuum(connection);
        }
    }
}
