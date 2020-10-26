// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ISQLiteIndex.h"
#include "Microsoft/Schema/1_0/SearchResultsTable.h"

#include <memory>
#include <vector>


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    // Interface to this schema version exposed through ISQLiteIndex.
    struct Interface : public ISQLiteIndex
    {
        // Version 1.0
        Schema::Version GetVersion() const override;
        void CreateTables(SQLite::Connection& connection) override;
        SQLite::rowid_t AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) override;
        std::pair<bool, SQLite::rowid_t> UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) override;
        SQLite::rowid_t RemoveManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) override;
        void PrepareForPackaging(SQLite::Connection& connection) override;
        bool CheckConsistency(const SQLite::Connection& connection, bool log) const override;
        SearchResult Search(const SQLite::Connection& connection, const SearchRequest& request) const override;
        std::optional<std::string> GetPropertyByManifestId(const SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionProperty property) const override;
        std::vector<std::string> GetMultiPropertyByManifestId(const SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionMultiProperty property) const override;
        std::optional<SQLite::rowid_t> GetManifestIdByKey(const SQLite::Connection& connection, SQLite::rowid_t id, std::string_view version, std::string_view channel) const override;
        std::vector<Utility::VersionAndChannel> GetVersionKeysById(const SQLite::Connection& connection, SQLite::rowid_t id) const override;

        // Version 1.1
        MetadataResult GetMetadataByManifestId(const SQLite::Connection& connection, SQLite::rowid_t manifestId) const override;
        void SetMetadataByManifestId(SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionMetadata metadata, std::string_view value) override;

    protected:
        // Creates the search results table.
        virtual std::unique_ptr<SearchResultsTable> CreateSearchResultsTable(const SQLite::Connection& connection) const;

        // Gets the ordering of matches to execute, with more specific matches coming first.
        virtual std::vector<MatchType> GetMatchTypeOrder(MatchType type) const;

        // Executes all relevant searchs for the query.
        virtual void PerformQuerySearch(SearchResultsTable& resultsTable, const RequestMatch& query) const;
    };
}
