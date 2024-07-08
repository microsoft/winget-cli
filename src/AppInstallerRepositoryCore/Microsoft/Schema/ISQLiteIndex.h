// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteWrapper.h>
#include <winget/SQLiteVersion.h>
#include "ISource.h"
#include "Microsoft/Schema/SQLiteIndexContextData.h"
#include <AppInstallerVersions.h>
#include <winget/Manifest.h>
#include <winget/NameNormalization.h>

#include <filesystem>
#include <optional>


namespace AppInstaller::Repository::Microsoft::Schema
{
    // Contains the database connection and any other data that the owning index might need to pass in.
    struct SQLiteIndexContext
    {
        SQLite::Connection& Connection;
        SQLiteIndexContextData& Data;
    };

    // The common interface used to interact with all schema versions of the index.
    struct ISQLiteIndex
    {
        virtual ~ISQLiteIndex() = default;

        // The non-version specific return value of Search.
        // New fields must have initializers to their down-schema defaults.
        struct SearchResult
        {
            std::vector<std::pair<SQLite::rowid_t, PackageMatchFilter>> Matches;
            bool Truncated = false;
        };

        // The non-version specific return value of GetMetadataByManifestId.
        using MetadataResult = std::vector<std::pair<PackageVersionMetadata, std::string>>;

        // Version 1.0

        // Gets the schema version that this index interface is built for.
        virtual SQLite::Version GetVersion() const = 0;

        // Options for creating the index.
        enum class CreateOptions
        {
            // Standard
            None = 0x0,
            // Enable support for passing in nullopt values to Add/UpdateManifest
            SupportPathless = 0x1,
            // Disable support for dependencies
            DisableDependenciesSupport = 0x2,
        };

        // Contains both the object representation of the version key and the rows.
        struct VersionKey
        {
            Utility::VersionAndChannel VersionAndChannel;
            SQLite::rowid_t ManifestId;

            bool operator<(const VersionKey& other) const { return VersionAndChannel < other.VersionAndChannel; }
        };

        // Creates all of the version dependent tables within the database.
        virtual void CreateTables(SQLite::Connection& connection, CreateOptions options) = 0;

        // Adds the manifest at the repository relative path to the index.
        virtual SQLite::rowid_t AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath) = 0;

        // Updates the manifest with matching { Id, Version, Channel } in the index.
        // The return value indicates whether the index was modified by the function.
        virtual std::pair<bool, SQLite::rowid_t> UpdateManifest(
            SQLite::Connection& connection,
            const Manifest::Manifest& manifest,
            const std::optional<std::filesystem::path>& relativePath) = 0;

        // Removes the manifest with matching { Id, Version, Channel } from the index.
        virtual SQLite::rowid_t RemoveManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest) = 0;

        // Removes the manifest with the given id.
        virtual void RemoveManifestById(SQLite::Connection& connection, SQLite::rowid_t manifestId) = 0;

        // Removes data that is no longer needed for an index that is to be published.
        virtual void PrepareForPackaging(SQLite::Connection& connection) = 0;

        // Removes data that is no longer needed for an index that is to be published.
        virtual void PrepareForPackaging(const SQLiteIndexContext& context);

        // Checks the consistency of the index to ensure that every referenced row exists.
        // Returns true if index is consistent; false if it is not.
        virtual bool CheckConsistency(const SQLite::Connection& connection, bool log) const = 0;

        // Performs a search based on the given criteria.
        virtual SearchResult Search(const SQLite::Connection& connection, const SearchRequest& request) const = 0;

        // Gets the string for the given property and primary id, if present.
        virtual std::optional<std::string> GetPropertyByPrimaryId(const SQLite::Connection& connection, SQLite::rowid_t primaryId, PackageVersionProperty property) const = 0;

        // Gets the string values for the given property and primary id, if present.
        virtual std::vector<std::string> GetMultiPropertyByPrimaryId(const SQLite::Connection& connection, SQLite::rowid_t primaryId, PackageVersionMultiProperty property) const = 0;

        // Gets the manifest id for the given { id, version, channel }, if present.
        // If version is empty, gets the value for the 'latest' version.
        virtual std::optional<SQLite::rowid_t> GetManifestIdByKey(const SQLite::Connection& connection, SQLite::rowid_t id, std::string_view version, std::string_view channel) const = 0;

        // Gets the manifest id for the given manifest, if present.
        virtual std::optional<SQLite::rowid_t> GetManifestIdByManifest(const SQLite::Connection& connection, const Manifest::Manifest& manifest) const = 0;

        // Gets all versions and channels for the given id.
        virtual std::vector<VersionKey> GetVersionKeysById(const SQLite::Connection& connection, SQLite::rowid_t id) const = 0;

        // Version 1.1

        // Gets the string for the given metadata and manifest id, if present.
        virtual MetadataResult GetMetadataByManifestId(const SQLite::Connection& connection, SQLite::rowid_t manifestId) const = 0;

        // Sets the string for the given metadata and manifest id.
        virtual void SetMetadataByManifestId(SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionMetadata metadata, std::string_view value) = 0;

        // Version 1.2

        // Normalizes a name using the internal rules used by the index.
        // Largely a utility function; should not be used to do work on behalf of the index by the caller.
        virtual Utility::NormalizedName NormalizeName(std::string_view name, std::string_view publisher) const = 0;

        // Version 1.4

        // Get all the dependencies for a specific manifest.
        virtual std::set<std::pair<SQLite::rowid_t, Utility::NormalizedString>> GetDependenciesByManifestRowId(const SQLite::Connection& connection, SQLite::rowid_t manifestRowId) const = 0;

        virtual std::vector<std::pair<SQLite::rowid_t, Utility::NormalizedString>> GetDependentsById(const SQLite::Connection& connection, AppInstaller::Manifest::string_t packageId) const = 0;

        // Version 1.7

        // Drops all tables that would have been created.
        virtual void DropTables(SQLite::Connection& connection) = 0;

        // Version 2.0

        // Migrates from the current interface given.
        // Returns true if supported; false if not.
        // Throws on errors that occur during an attempted migration.
        virtual bool MigrateFrom(SQLite::Connection& connection, const ISQLiteIndex* current) = 0;

        // Set the property value.
        virtual void SetProperty(SQLite::Connection& connection, Property property, const std::string& value);
    };

    DEFINE_ENUM_FLAG_OPERATORS(ISQLiteIndex::CreateOptions);

    // Creates the ISQLiteIndex interface object for the given version.
    std::unique_ptr<ISQLiteIndex> CreateISQLiteIndex(const SQLite::Version& version);

    // For a given match type, gets the set of match types that are more specific subsets of it.
    std::vector<MatchType> GetDefaultMatchTypeOrder(MatchType type);
}
