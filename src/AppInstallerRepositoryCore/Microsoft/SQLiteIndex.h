// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/Version.h"
#include "Manifest/Manifest.h"

#include <filesystem>
#include <limits>
#include <memory>
#include <string>

namespace AppInstaller::Repository::Microsoft
{
    // Holds the connection to the database, as well as the appropriate functionality to interface with it.
    struct SQLiteIndex
    {
        SQLiteIndex(const SQLiteIndex&) = delete;
        SQLiteIndex& operator=(const SQLiteIndex&) = delete;

        SQLiteIndex(SQLiteIndex&&) = default;
        SQLiteIndex& operator=(SQLiteIndex&&) = default;

        // Creates a new index database of the given version.
        static SQLiteIndex CreateNew(const std::string& filePath, Schema::Version version);

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

        // Adds the manifest at the repository relative path to the index.
        void AddManifest(const std::filesystem::path& manifestPath, const std::filesystem::path& relativePath);

        // Adds the manifest at the repository relative path to the index.
        void AddManifest(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath);

        // Updates the manifest at the repository relative path in the index.
        // If the old manifest does not exist in the index, this is equivalent to AddManifest(newManifest, newRelativePath).
        void UpdateManifest(const std::filesystem::path& oldManifestPath, const std::filesystem::path& oldRelativePath, const std::filesystem::path& newManifestPath, const std::filesystem::path& newRelativePath);

        // Updates the manifest at the repository relative path in the index.
        // If the old manifest does not exist in the index, this is equivalent to AddManifest(newManifest, newRelativePath).
        void UpdateManifest(const Manifest::Manifest& oldManifest, const std::filesystem::path& oldRelativePath, const Manifest::Manifest& newManifest, const std::filesystem::path& newRelativePath);

        // Removes the manifest at the repository relative path from the index.
        void RemoveManifest(const std::filesystem::path& manifestPath, const std::filesystem::path& relativePath);

        // Removes the manifest at the repository relative path from the index.
        void RemoveManifest(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath);

    private:
        // Constructor used to open an existing index.
        SQLiteIndex(const std::string& target, SQLite::Connection::OpenDisposition disposition, SQLite::Connection::OpenFlags flags);

        // Constructor used to create a new index.
        SQLiteIndex(const std::string& target, Schema::Version version);

        SQLite::Connection m_dbconn;
        Schema::Version m_version;
        std::unique_ptr<Schema::ISQLiteIndex> m_interface;
    };
}
