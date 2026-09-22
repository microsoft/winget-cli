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
        // Determines how the removal of a package is reflected in the table.
        enum class RemovalBehavior
        {
            Delete,
            Record,
        };

        // Get the table name.
        static std::string_view TableName();

        // Creates the table.
        static void Create(SQLite::Connection& connection, RemovalBehavior removals);

        // Creates the unique index that allows at most one live row per package rowid.
        static void CreateLiveRowIndex(SQLite::Connection& connection);

        // Creates the index over the change sequence.
        static void CreateChangeSequenceIndex(SQLite::Connection& connection);

        // Creates the table if it does not exist.
        static void EnsureExists(SQLite::Connection& connection, RemovalBehavior removals);

        // Drops the table.
        static void Drop(SQLite::Connection& connection);

        // Determine if the table currently exists in the database.
        static bool Exists(const SQLite::Connection& connection);

        // Updates the tracking table for the given package identifier in the internal index.
        static void Update(SQLite::Connection& connection, const ISQLiteIndex* internalIndex, const std::string& packageIdentifier, RemovalBehavior removals, bool ensureTable = true, std::optional<SQLite::rowid_t> removedPackageRowId = {});

        // Checks the consistency of the index to ensure that every referenced row exists.
        // Returns true if index is consistent; false if it is not.
        static bool CheckConsistency(const SQLite::Connection& connection, ISQLiteIndex* internalIndex, RemovalBehavior removals, bool log);

        // Data on a single row in the table.
        struct PackageData
        {
            SQLite::rowid_t RowID = 0;
            std::string PackageIdentifier;
            int64_t WriteTime = 0;
            SQLite::blob_t Manifest;
            SQLite::blob_t Hash;
            // The rowid the package occupies in the index, or 0 when it is not known.
            SQLite::rowid_t PackageRowId = 0;
        };

        // Gets the data on updates that have been written since the given base time.
        // Removed packages are never included; use GetRemovalsSince to retrieve those.
        static std::vector<PackageData> GetUpdatesSince(const SQLite::Connection& connection, int64_t updateBaseTime, RemovalBehavior removals);

        // Gets the rowids vacated by packages removed since the given base time.
        // Only meaningful when removals are being recorded; always empty otherwise.
        static std::set<SQLite::rowid_t> GetRemovalsSince(const SQLite::Connection& connection, int64_t updateBaseTime, RemovalBehavior removals);

        // Gets the data on updates written after the given change sequence, exclusive.
        // Only available when removals are being recorded, as only then does the column exist.
        static std::vector<PackageData> GetUpdatesSinceSequence(const SQLite::Connection& connection, int64_t baseSequence, RemovalBehavior removals);

        // Gets the rowids vacated by packages removed after the given change sequence, exclusive.
        // Only available when removals are being recorded, as only then does the column exist.
        static std::set<SQLite::rowid_t> GetRemovalsSinceSequence(const SQLite::Connection& connection, int64_t baseSequence, RemovalBehavior removals);

        // Gets the most recently issued change sequence, or 0 if nothing has been written.
        // Only available when removals are being recorded, as only then does the column exist.
        static int64_t GetCurrentChangeSequence(const SQLite::Connection& connection, RemovalBehavior removals);

        // Gets the data hash for the given package identifier.
        static SQLite::blob_t GetDataHash(const SQLite::Connection& connection, const std::string& packageIdentifier, RemovalBehavior removals);

        // Rebuilds an existing 2.0 table in the shape that recording removals requires.
        // The table is rebuilt rather than altered because a tombstone has no manifest or hash,
        // and SQLite cannot drop the not null constraint that a 2.0 table places on them.
        static void MigrateToRemovalTracking(SQLite::Connection& connection);
    };
}
