// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/ISQLiteIndex.h"
#include "AppInstallerRepositorySearch.h"

#include <string>
#include <string_view>
#include <vector>


namespace AppInstaller::Repository::Microsoft::Schema::V1_1
{
    // A table for storing arbitrary metadata on idividual manifests.
    // The table and all metadata are optional.
    struct ManifestMetadataTable
    {
        // Determine if the table currently exists in the database.
        static bool Exists(const SQLite::Connection& connection);

        // Creates the table in the database.
        static void Create(SQLite::Connection& connection);

        // Gets all metadata associated with the given manifest.
        // The table must exist.
        static ISQLiteIndex::MetadataResult GetMetadataByManifestId(const SQLite::Connection& connection, SQLite::rowid_t manifestId);

        // Sets the metadata value for the given manifest.
        // The table must exist.
        static void SetMetadataByManifestId(SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionMetadata metadata, std::string_view value);

        // Removes all metadata values for the given manifest.
        // The table must exist.
        static void DeleteByManifestId(SQLite::Connection& connection, SQLite::rowid_t manifestId);
    };
}
