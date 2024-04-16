// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ISQLiteIndex.h"
#include <winget/SQLiteWrapper.h>


namespace AppInstaller::Repository::Microsoft::Schema::V2_0
{
    // Table for tracking the updates to the internal table so that prepare can output
    // only the necessary package manifests.
    struct PackageUpdateTrackingTable
    {
        // Get the table name.
        static std::string_view TableName();

        // Creates the table.
        static void Create(SQLite::Connection& connection);

        // Creates the table if it does not exist.
        static void EnsureExists(SQLite::Connection& connection);

        // Drops the table.
        static void Drop(SQLite::Connection& connection);

        // Determine if the table currently exists in the database.
        static bool Exists(const SQLite::Connection& connection);

        // Updates the tracking table for the given package identifier in the internal index.
        static void Update(SQLite::Connection& connection, const ISQLiteIndex* internalIndex, const std::string& packageIdentifier, bool ensureTable = true);

        // Checks the consistency of the index to ensure that every referenced row exists.
        // Returns true if index is consistent; false if it is not.
        static bool CheckConsistency(const SQLite::Connection& connection, ISQLiteIndex* internalIndex, bool log);

        // Data on a single row in the table.
        struct PackageData
        {
            SQLite::rowid_t RowID = 0;
            std::string PackageIdentifier;
            int64_t WriteTime = 0;
            SQLite::blob_t Manifest;
            SQLite::blob_t Hash;
        };

        // Gets the data on updates that have been written since the given base time.
        static std::vector<PackageData> GetUpdatesSince(const SQLite::Connection& connection, int64_t updateBaseTime);

        // Gets the data hash for the given package identifier.
        static SQLite::blob_t GetDataHash(const SQLite::Connection& connection, const std::string& packageIdentifier);
    };
}
