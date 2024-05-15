// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ISQLiteIndex.h"
#include "Microsoft/Schema/1_5/Interface.h"

namespace AppInstaller::Repository::Microsoft::Schema::V1_6
{
    // Interface to this schema version exposed through ISQLiteIndex.
    struct Interface : public V1_5::Interface
    {
        Interface(Utility::NormalizationVersion normVersion = Utility::NormalizationVersion::Initial);

        // Version 1.0
        SQLite::Version GetVersion() const override;
        void CreateTables(SQLite::Connection& connection, CreateOptions options) override;
        SQLite::rowid_t AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath) override;
        std::pair<bool, SQLite::rowid_t> UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath) override;
        void RemoveManifestById(SQLite::Connection& connection, SQLite::rowid_t manifestId) override;
        bool CheckConsistency(const SQLite::Connection& connection, bool log) const override;
        std::vector<std::string> GetMultiPropertyByPrimaryId(const SQLite::Connection& connection, SQLite::rowid_t primaryId, PackageVersionMultiProperty property) const override;

        // Version 1.7
        void DropTables(SQLite::Connection& connection) override;

    protected:
        std::unique_ptr<V1_0::SearchResultsTable> CreateSearchResultsTable(const SQLite::Connection& connection) const override;
        void PerformQuerySearch(V1_0::SearchResultsTable& resultsTable, const RequestMatch& query) const override;
        ISQLiteIndex::SearchResult SearchInternal(const SQLite::Connection& connection, SearchRequest& request) const;
        void PrepareForPackaging(SQLite::Connection& connection, bool vacuum) override;
    };
}
