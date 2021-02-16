// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_2/Interface.h"

#include "Microsoft/Schema/1_2/SearchResultsTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_2
{
    namespace
    {
        // TODO: Adapt algorithm to get name and publisher from all localizations
        std::vector<Utility::NormalizedString> GetSystemReferenceStrings(
            const Manifest::Manifest& manifest,
            std::function<const Utility::NormalizedString&(const Manifest::ManifestInstaller&)> func)
        {
            std::set<Utility::NormalizedString> set;

            for (const auto& installer : manifest.Installers)
            {
                const Utility::NormalizedString& string = func(installer);
                if (!string.empty())
                {
                    set.emplace(Utility::FoldCase(string));
                }
            }

            std::vector<Utility::NormalizedString> result;
            for (auto&& string : set)
            {
                result.emplace_back(string);
            }

            return result;
        }

        std::vector<Utility::NormalizedString> GetPackageFamilyNames(const Manifest::Manifest& manifest)
        {
            return GetSystemReferenceStrings(manifest, [](const Manifest::ManifestInstaller& i) -> const Utility::NormalizedString& { return i.PackageFamilyName; });
        }

        std::vector<Utility::NormalizedString> GetProductCodes(const Manifest::Manifest& manifest)
        {
            return GetSystemReferenceStrings(manifest, [](const Manifest::ManifestInstaller& i) -> const Utility::NormalizedString& { return i.ProductCode; });
        }
    }

    Schema::Version Interface::GetVersion() const
    {
        return { 1, 2 };
    }

    void Interface::CreateTables(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createtables_v1_2");

        V1_1::Interface::CreateTables(connection);

        // TODO: Create normalized name and publisher table

        savepoint.Commit();
    }

    SQLite::rowid_t Interface::AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addmanifest_v1_2");

        SQLite::rowid_t manifestId = V1_1::Interface::AddManifest(connection, manifest, relativePath);

        // Add the new 1.2 data
        // TODO: This

        savepoint.Commit();

        return manifestId;
    }

    std::pair<bool, SQLite::rowid_t> Interface::UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "updatemanifest_v1_2");

        auto [indexModified, manifestId] = V1_1::Interface::UpdateManifest(connection, manifest, relativePath);

        // Update new 1.2 tables as necessary
        indexModified = PackageFamilyNameTable::UpdateIfNeededByManifestId(connection, GetPackageFamilyNames(manifest), manifestId) || indexModified;
        indexModified = ProductCodeTable::UpdateIfNeededByManifestId(connection, GetProductCodes(manifest), manifestId) || indexModified;

        savepoint.Commit();

        return { indexModified, manifestId };
    }

    SQLite::rowid_t Interface::RemoveManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "removemanifest_v1_2");

        SQLite::rowid_t manifestId = V1_1::Interface::RemoveManifest(connection, manifest, relativePath);

        // Remove all of the new 1.2 data that is no longer referenced.
        PackageFamilyNameTable::DeleteIfNotNeededByManifestId(connection, manifestId);
        ProductCodeTable::DeleteIfNotNeededByManifestId(connection, manifestId);

        if (ManifestMetadataTable::Exists(connection))
        {
            ManifestMetadataTable::DeleteByManifestId(connection, manifestId);
        }

        savepoint.Commit();

        return manifestId;
    }

    void Interface::PrepareForPackaging(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "prepareforpackaging_v1_2");

        // TODO: allow passing this bool to disable the vacuum
        V1_1::Interface::PrepareForPackaging(connection, false);

        // TODO: Prepare the new table

        savepoint.Commit();

        // Force the database to actually shrink the file size.
        // This *must* be done outside of an active transaction.
        SQLite::Builder::StatementBuilder builder;
        builder.Vacuum();
        builder.Execute(connection);
    }

    bool Interface::CheckConsistency(const SQLite::Connection& connection, bool log) const
    {
        bool result = V1_1::Interface::CheckConsistency(connection, log);

        // If the v1.1 index was consistent, or if full logging of inconsistency was requested, check the v1.2 data.
        if (result || log)
        {
            result = PackageFamilyNameTable::CheckConsistency(connection, log) && result;
        }

        if (result || log)
        {
            result = ProductCodeTable::CheckConsistency(connection, log) && result;
        }

        return result;
    }

    ISQLiteIndex::SearchResult Interface::Search(const SQLite::Connection& connection, const SearchRequest& request) const
    {
        // TODO: Update for 1.2 data
        // Update any system reference strings to be folded
        SearchRequest foldedRequest = request;

        auto foldIfNeeded = [](PackageMatchFilter& filter)
        {
            if ((filter.Field == PackageMatchField::PackageFamilyName || filter.Field == PackageMatchField::ProductCode) &&
                filter.Type == MatchType::Exact)
            {
                filter.Value = Utility::FoldCase(filter.Value);
            }
        };

        for (auto& inclusion : foldedRequest.Inclusions)
        {
            foldIfNeeded(inclusion);
        }

        for (auto& filter : foldedRequest.Filters)
        {
            foldIfNeeded(filter);
        }

        return V1_1::Interface::Search(connection, foldedRequest);
    }

    std::unique_ptr<V1_0::SearchResultsTable> Interface::CreateSearchResultsTable(const SQLite::Connection& connection) const
    {
        return std::make_unique<SearchResultsTable>(connection);
    }

    void Interface::PerformQuerySearch(V1_0::SearchResultsTable& resultsTable, const RequestMatch& query) const
    {
        // First, do an exact match search for the folded system reference strings
        // We do this first because it is exact, and likely won't match anything else if it matches this.
        std::string foldedQuery = Utility::FoldCase(query.Value);
        resultsTable.SearchOnField(PackageMatchField::PackageFamilyName, MatchType::Exact, foldedQuery);
        resultsTable.SearchOnField(PackageMatchField::ProductCode, MatchType::Exact, foldedQuery);

        // Then do the 1.0 search
        V1_0::Interface::PerformQuerySearch(resultsTable, query);
    }
}
