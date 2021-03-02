// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/Version.h"
#include "Public/AppInstallerRepositorySearch.h"
#include <AppInstallerVersions.h>
#include <winget/Manifest.h>
#include <winget/NameNormalization.h>

#include <filesystem>


namespace AppInstaller::Repository::Microsoft::Schema
{
    // Forward declarations
    struct Version;

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
        virtual Schema::Version GetVersion() const = 0;

        // Creates all of the version dependent tables within the database.
        virtual void CreateTables(SQLite::Connection& connection) = 0;

        // Adds the manifest at the repository relative path to the index.
        virtual SQLite::rowid_t AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) = 0;

        // Updates the manifest with matching { Id, Version, Channel } in the index.
        // The return value indicates whether the index was modified by the function.
        virtual std::pair<bool, SQLite::rowid_t> UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) = 0;

        // Removes the manifest with matching { Id, Version, Channel } from the index.
        // Path is currently ignored.
        virtual SQLite::rowid_t RemoveManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) = 0;

        // Removes data that is no longer needed for an index that is to be published.
        virtual void PrepareForPackaging(SQLite::Connection& connection) = 0;

        // Checks the consistency of the index to ensure that every referenced row exists.
        // Returns true if index is consistent; false if it is not.
        virtual bool CheckConsistency(const SQLite::Connection& connection, bool log) const = 0;

        // Performs a search based on the given criteria.
        virtual SearchResult Search(const SQLite::Connection& connection, const SearchRequest& request) const = 0;

        // Gets the string for the given property and manifest id, if present.
        virtual std::optional<std::string> GetPropertyByManifestId(const SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionProperty property) const = 0;

        // Gets the string values for the given property and manifest id, if present.
        virtual std::vector<std::string> GetMultiPropertyByManifestId(const SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionMultiProperty property) const = 0;

        // Gets the manifest id for the given { id, version, channel }, if present.
        // If version is empty, gets the value for the 'latest' version.
        virtual std::optional<SQLite::rowid_t> GetManifestIdByKey(const SQLite::Connection& connection, SQLite::rowid_t id, std::string_view version, std::string_view channel) const = 0;

        // Gets all versions and channels for the given id.
        virtual std::vector<Utility::VersionAndChannel> GetVersionKeysById(const SQLite::Connection& connection, SQLite::rowid_t id) const = 0;

        // Version 1.1

        // Gets the string for the given metadata and manifest id, if present.
        virtual MetadataResult GetMetadataByManifestId(const SQLite::Connection& connection, SQLite::rowid_t manifestId) const = 0;

        // Sets the string for the given metadata and manifest id.
        virtual void SetMetadataByManifestId(SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionMetadata metadata, std::string_view value) = 0;

        // Normalizes a name using the internal rules used by the index.
        // Largely a utility function; should not be used to do work on behalf of the index by the caller.
        virtual Utility::NormalizedName NormalizeName(std::string_view name, std::string_view publisher) const = 0;
    };
}
