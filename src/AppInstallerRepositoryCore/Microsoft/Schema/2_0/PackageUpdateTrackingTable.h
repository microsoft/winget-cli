// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ISQLiteIndex.h"
#include <winget/SQLiteWrapper.h>

#include <set>


namespace AppInstaller::Repository::Microsoft::Schema::V2_0
{
    // Table for tracking the updates to the internal table so that prepare can output
    // only the necessary package manifests.
    struct PackageUpdateTrackingTable
    {
        // Determines how the removal of a package is reflected in the table.
        // Schema 2.0 deletes the row outright, so the table has no record that the package
        // ever existed. Schema 2.1 instead marks the row as removed, which is what allows a
        // delta index to express a removal; that requires the `is_removed` column, which only
        // exists on tables created or migrated by 2.1.
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
        // When the package is no longer present and removals are being recorded, removedPackageRowId
        // must carry the rowid it occupied; the caller has to resolve that before removing it from
        // the index, as the ids row is gone by the time this is called.
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
            // Only recorded when removals are being recorded; see the column comment.
            SQLite::rowid_t PackageRowId = 0;
        };

        // Gets the data on updates that have been written since the given base time.
        // Removed packages are never included; use GetRemovalsSince to retrieve those.
        static std::vector<PackageData> GetUpdatesSince(const SQLite::Connection& connection, int64_t updateBaseTime, RemovalBehavior removals);

        // Gets the rowids vacated by packages removed since the given base time.
        // Only meaningful when removals are being recorded; always empty otherwise.
        // The rowid is reported rather than the identifier because it is the identity a delta is
        // keyed on and the only one that can be compared exactly; see the implementation.
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

        // Adds the columns needed to record removals to an existing table that does not have them,
        // and backfills the package rowid for the rows already present.
        // Used when migrating from schema 2.0 to 2.1; does nothing if the table does not exist.
        static void AddRemovalTrackingColumns(SQLite::Connection& connection);
    };
}
