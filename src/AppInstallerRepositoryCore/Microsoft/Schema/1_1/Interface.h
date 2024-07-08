// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ISQLiteIndex.h"
#include "Microsoft/Schema/1_0/Interface.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_1
{
    // Interface to this schema version exposed through ISQLiteIndex.
    struct Interface : public V1_0::Interface
    {
        // Version 1.0
        SQLite::Version GetVersion() const override;
        void CreateTables(SQLite::Connection& connection, CreateOptions options) override;
        SQLite::rowid_t AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath) override;
        std::pair<bool, SQLite::rowid_t> UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath) override;
        void RemoveManifestById(SQLite::Connection& connection, SQLite::rowid_t manifestId) override;
        void PrepareForPackaging(SQLite::Connection& connection) override;
        bool CheckConsistency(const SQLite::Connection& connection, bool log) const override;
        SearchResult Search(const SQLite::Connection& connection, const SearchRequest& request) const override;
        std::vector<std::string> GetMultiPropertyByPrimaryId(const SQLite::Connection& connection, SQLite::rowid_t primaryId, PackageVersionMultiProperty property) const override;

        // Version 1.1
        MetadataResult GetMetadataByManifestId(const SQLite::Connection& connection, SQLite::rowid_t manifestId) const override;
        void SetMetadataByManifestId(SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionMetadata metadata, std::string_view value) override;

        // Version 1.7
        void DropTables(SQLite::Connection& connection) override;

    protected:
        std::unique_ptr<V1_0::SearchResultsTable> CreateSearchResultsTable(const SQLite::Connection& connection) const override;
        void PerformQuerySearch(V1_0::SearchResultsTable& resultsTable, const RequestMatch& query) const override;
        V1_0::OneToManyTableSchema GetOneToManyTableSchema() const override;

        virtual SearchResult SearchInternal(const SQLite::Connection& connection, SearchRequest& request) const;
        virtual void PrepareForPackaging(SQLite::Connection& connection, bool vacuum);

        // Gets a property already knowing that the manifest id is valid.
        virtual std::optional<std::string> GetPropertyByManifestIdInternal(const SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionProperty property) const;
    };
}
