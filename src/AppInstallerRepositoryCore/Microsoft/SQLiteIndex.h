// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/ISQLiteIndex.h"
#include "Microsoft/Schema/Version.h"
#include "Public/AppInstallerRepositorySearch.h"
#include <AppInstallerLanguageUtilities.h>
#include <AppInstallerVersions.h>
#include <winget/Manifest.h>
#include <winget/NameNormalization.h>

#include <chrono>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace AppInstaller::Repository::Microsoft
{
    // Holds the connection to the database, as well as the appropriate functionality to interface with it.
    struct SQLiteIndex
    {
        // An id that refers to a specific application.
        using IdType = SQLite::rowid_t;

        // The return type of Search
        using SearchResult = Schema::ISQLiteIndex::SearchResult;

        // The return type of GetMetadataByManifestId
        using MetadataResult = Schema::ISQLiteIndex::MetadataResult;

        SQLiteIndex(const SQLiteIndex&) = delete;
        SQLiteIndex& operator=(const SQLiteIndex&) = delete;

        SQLiteIndex(SQLiteIndex&&) = default;
        SQLiteIndex& operator=(SQLiteIndex&&) = default;

        // Creates a new index database of the given version.
        static SQLiteIndex CreateNew(const std::string& filePath, Schema::Version version = Schema::Version::Latest());

        // The disposition for opening the index.
        enum class OpenDisposition
        {
            // Open for read only.
            Read,
            // Open for read and write.
            ReadWrite,
            // The database will not change while in use; open for immutable read.
            Immutable,
        };

        // Opens an existing index database.
        static SQLiteIndex Open(const std::string& filePath, OpenDisposition disposition);

        // Gets the schema version of the index.
        Schema::Version GetVersion() const { return m_version; }

#ifndef AICLI_DISABLE_TEST_HOOKS
        // Changes the version of the interface being used to operate on the database.
        // Should only be used for testing.
        void ForceVersion(const Schema::Version& version);
#endif

        // Gets the last write time for the index.
        std::chrono::system_clock::time_point GetLastWriteTime();

        // Adds the manifest at the repository relative path to the index.
        // If the function succeeds, the manifest has been added.
        // Returns the manifest id.
        IdType AddManifest(const std::filesystem::path& manifestPath, const std::filesystem::path& relativePath);

        // Adds the manifest at the repository relative path to the index.
        // If the function succeeds, the manifest has been added.
        // Returns the manifest id.
        IdType AddManifest(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath);

        // Updates the manifest with matching { Id, Version, Channel } in the index.
        // The return value indicates whether the index was modified by the function.
        bool UpdateManifest(const std::filesystem::path& manifestPath, const std::filesystem::path& relativePath);

        // Updates the manifest with matching { Id, Version, Channel } in the index.
        // The return value indicates whether the index was modified by the function.
        bool UpdateManifest(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath);

        // Removes the manifest with matching { Id, Version, Channel } from the index.
        // Path is currently ignored.
        void RemoveManifest(const std::filesystem::path& manifestPath, const std::filesystem::path& relativePath);

        // Removes the manifest with matching { Id, Version, Channel } from the index.
        // Path is currently ignored.
        void RemoveManifest(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath);

        // Removes data that is no longer needed for an index that is to be published.
        void PrepareForPackaging();

        // Checks the consistency of the index to ensure that every referenced row exists.
        // Returns true if index is consistent; false if it is not.
        bool CheckConsistency(bool log = false) const;

        // Performs a search based on the given criteria.
        SearchResult Search(const SearchRequest& request) const;

        // Gets the string for the given property and manifest id, if present.
        std::optional<std::string> GetPropertyByManifestId(IdType manifestId, PackageVersionProperty property) const;

        // Gets the string values for the given property and manifest id, if present.
        std::vector<std::string> GetMultiPropertyByManifestId(IdType manifestId, PackageVersionMultiProperty property) const;

        // Gets the manifest id for the given { id, version, channel }, if present.
        // If version is empty, gets the value for the 'latest' version.
        std::optional<IdType> GetManifestIdByKey(IdType id, std::string_view version, std::string_view channel) const;

        // Gets all versions and channels for the given id.
        std::vector<Utility::VersionAndChannel> GetVersionKeysById(IdType id) const;

        // Gets the string for the given metadata and manifest id, if present.
        MetadataResult GetMetadataByManifestId(SQLite::rowid_t manifestId) const;

        // Sets the string for the given metadata and manifest id.
        void SetMetadataByManifestId(IdType manifestId, PackageVersionMetadata metadata, std::string_view value);

        // Normalizes a name using the internal rules used by the index.
        // Largely a utility function; should not be used to do work on behalf of the index by the caller.
        Utility::NormalizedName NormalizeName(std::string_view name, std::string_view publisher) const;

    private:
        // Constructor used to open an existing index.
        SQLiteIndex(const std::string& target, SQLite::Connection::OpenDisposition disposition, SQLite::Connection::OpenFlags flags);

        // Constructor used to create a new index.
        SQLiteIndex(const std::string& target, Schema::Version version);

        // Sets the last write time metadata value in the index.
        void SetLastWriteTime();

        SQLite::Connection m_dbconn;
        Schema::Version m_version;
        std::unique_ptr<Schema::ISQLiteIndex> m_interface;
    };
}
