// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/Version.h"
#include "Manifest/Manifest.h"

#include <filesystem>


namespace AppInstaller::Repository::Microsoft::Schema
{
    // Forward declarations
    struct Version;

    // The common interface used to interact with all schema versions of the index.
    struct ISQLiteIndex
    {
        virtual ~ISQLiteIndex() = default;

        // Version 1.0

        // Gets the schema version that this index interface is built for.
        virtual Schema::Version GetVersion() const = 0;

        // Creates all of the version dependent tables within the database.
        virtual void CreateTables(SQLite::Connection& connection) = 0;

        // Adds the manifest at the repository relative path to the index.
        virtual void AddManifest(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) = 0;

        // Updates the manifest at the repository relative path in the index.
        // If the old manifest does not exist in the index, this is equivalent to AddManifest(newManifest, newRelativePath).
        virtual void UpdateManifest(const Manifest::Manifest& oldManifest, const std::filesystem::path& oldRelativePath, const Manifest::Manifest& newManifest, const std::filesystem::path& newRelativePath) = 0;

        // Removes the manifest at the repository relative path from the index.
        virtual void RemoveManifest(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) = 0;
    };


    // Common base class used by all schema versions.
    struct SQLiteIndexBase : public ISQLiteIndex
    {
        SQLiteIndexBase(SQLite::Connection& connection) : m_connection(connection) {}

        SQLiteIndexBase(SQLiteIndexBase&) = delete;
        SQLiteIndexBase& operator=(SQLiteIndexBase&) = delete;

    protected:
        // Non-owning reference; the interface should never outlive the SQLiteIndex class.
        SQLite::Connection& m_connection;
    };
}
