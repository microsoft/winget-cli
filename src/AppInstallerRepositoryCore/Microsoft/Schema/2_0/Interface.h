// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ISQLiteIndex.h"
#include "Microsoft/Schema/2_0/SearchResultsTable.h"
#include "Microsoft/Schema/2_0/OneToManyTableWithMap.h"

#include <memory>
#include <vector>

using namespace std::string_view_literals;

namespace AppInstaller::Repository::Microsoft::Schema::V2_0
{
    // Version 2.0
    static constexpr std::string_view s_MetadataValueName_PackageUpdateTrackingBaseTime = "updateTrackingBase"sv;

    // Interface to this schema version exposed through ISQLiteIndex.
    struct Interface : public ISQLiteIndex
    {
        Interface(Utility::NormalizationVersion normVersion = Utility::NormalizationVersion::Initial);

        // Version 1.0
        SQLite::Version GetVersion() const override;
        void CreateTables(SQLite::Connection& connection, CreateOptions options) override;
        SQLite::rowid_t AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath) override;
        std::pair<bool, SQLite::rowid_t> UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath) override;
        SQLite::rowid_t RemoveManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest) override;
        void RemoveManifestById(SQLite::Connection& connection, SQLite::rowid_t manifestId) override;
        void PrepareForPackaging(SQLite::Connection& connection) override;
        void PrepareForPackaging(const SQLiteIndexContext& context) override;
        bool CheckConsistency(const SQLite::Connection& connection, bool log) const override;
        SearchResult Search(const SQLite::Connection& connection, const SearchRequest& request) const override;
        std::optional<std::string> GetPropertyByPrimaryId(const SQLite::Connection& connection, SQLite::rowid_t primaryId, PackageVersionProperty property) const override;
        std::vector<std::string> GetMultiPropertyByPrimaryId(const SQLite::Connection& connection, SQLite::rowid_t primaryId, PackageVersionMultiProperty property) const override;
        std::optional<SQLite::rowid_t> GetManifestIdByKey(const SQLite::Connection& connection, SQLite::rowid_t id, std::string_view version, std::string_view channel) const override;
        std::optional<SQLite::rowid_t> GetManifestIdByManifest(const SQLite::Connection& connection, const Manifest::Manifest& manifest) const override;
        std::vector<VersionKey> GetVersionKeysById(const SQLite::Connection& connection, SQLite::rowid_t id) const override;

        // Version 1.1
        MetadataResult GetMetadataByManifestId(const SQLite::Connection& connection, SQLite::rowid_t manifestId) const override;
        void SetMetadataByManifestId(SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionMetadata metadata, std::string_view value) override;

        // Version 1.2
        Utility::NormalizedName NormalizeName(std::string_view name, std::string_view publisher) const override;

        // Version 1.4 Get all the dependencies for a specific manifest.
        std::set<std::pair<SQLite::rowid_t, Utility::NormalizedString>> GetDependenciesByManifestRowId(const SQLite::Connection& connection, SQLite::rowid_t manifestRowId) const override;
        std::vector<std::pair<SQLite::rowid_t, Utility::NormalizedString>> GetDependentsById(const SQLite::Connection& connection, AppInstaller::Manifest::string_t packageId) const override;

        // Version 1.7
        void DropTables(SQLite::Connection& connection) override;

        // Version 2.0
        bool MigrateFrom(SQLite::Connection& connection, const ISQLiteIndex* current) override;
        void SetProperty(SQLite::Connection& connection, Property property, const std::string& value) override;

    protected:
        // Creates the search results table.
        virtual std::unique_ptr<SearchResultsTable> CreateSearchResultsTable(const SQLite::Connection& connection) const;

        // Executes all relevant searches for the query.
        virtual void PerformQuerySearch(SearchResultsTable& resultsTable, const RequestMatch& query) const;

        // Gets the one to many table schema to use.
        virtual OneToManyTableSchema GetOneToManyTableSchema() const;

        // Executes search on a request that can be modified.
        virtual SearchResult SearchInternal(const SQLite::Connection& connection, SearchRequest& request) const;

        // Executes search on the given request.
        SearchResult BasicSearchInternal(const SQLite::Connection& connection, const SearchRequest& request) const;

        // Prepares for packaging, optionally vacuuming the database.
        virtual void PrepareForPackaging(const SQLiteIndexContext& context, bool vacuum);

        // Force the database to shrink the file size.
        // This *must* be done outside of an active transaction.
        void Vacuum(const SQLite::Connection& connection);

        // If before PrepareForPackaging is called, this should find the internal interface schema version and create the object.
        // If after PrepareForPackaging is called, this should not find the internal interface schema version and allow the code to fall through.
        // requireInternalInterface should be set to true for modifying functions.
        void EnsureInternalInterface(const SQLite::Connection& connection, bool requireInternalInterface = false) const;

        // Allows derived types to move to a different internal schema version.
        virtual std::unique_ptr<Schema::ISQLiteIndex> CreateInternalInterface() const;

        // If EnsureInternalInterface has been called.
        mutable bool m_internalInterfaceChecked = false;

        // Interface to the data before PrepareForPackaging is called.
        mutable std::unique_ptr<Schema::ISQLiteIndex> m_internalInterface;

        // The name normalization utility
        Utility::NameNormalizer m_normalizer;
    };
}
