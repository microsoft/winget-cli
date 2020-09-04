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
        SearchResult Search(SQLite::Connection& connection, const SearchRequest& request) override;
        std::optional<std::string> GetIdStringById(SQLite::Connection& connection, SQLite::rowid_t id) override;
        std::optional<std::string> GetNameStringById(SQLite::Connection& connection, SQLite::rowid_t id) override;
        std::optional<std::string> GetPathStringByKey(SQLite::Connection& connection, SQLite::rowid_t id, std::string_view version, std::string_view channel) override;
        std::vector<Utility::VersionAndChannel> GetVersionsById(SQLite::Connection& connection, SQLite::rowid_t id) override;

    protected:
        // Creates the search results table.
        virtual std::unique_ptr<SearchResultsTable> CreateSearchResultsTable(SQLite::Connection& connection) const;

        // Gets the ordering of matches to execute, with more specific matches coming first.
        virtual std::vector<MatchType> GetMatchTypeOrder(MatchType type) const;

        // Executes all relevant searchs for the query.
        virtual void PerformQuerySearch(SearchResultsTable& resultsTable, const RequestMatch& query) const;
    };
}
