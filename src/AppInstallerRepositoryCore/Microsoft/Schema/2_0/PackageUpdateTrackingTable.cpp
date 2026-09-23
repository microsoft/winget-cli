// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PackageUpdateTrackingTable.h"
#include "Microsoft/Schema/1_0/IdTable.h"
#include <winget/PackageVersionDataManifest.h>
#include <winget/SQLiteStatementBuilder.h>
#include <AppInstallerStrings.h>

using namespace AppInstaller::SQLite;

namespace AppInstaller::Repository::Microsoft::Schema::V2_0
{
    using namespace std::string_view_literals;
    static constexpr std::string_view s_PUTT_Table_Name = "update_tracking"sv;
    static constexpr std::string_view s_PUTT_WriteTimeIndex_Name = "update_tracking_write_idx"sv;
    static constexpr std::string_view s_PUTT_LiveIndex_Name = "update_tracking_live_idx"sv;
    static constexpr std::string_view s_PUTT_Package = "package"sv;
    static constexpr std::string_view s_PUTT_WriteTime = "write_time"sv;
    static constexpr std::string_view s_PUTT_Manifest = "manifest"sv;
    static constexpr std::string_view s_PUTT_Hash = "hash"sv;
    // Only ever written as 0 or 1, but read as 0 versus not-0 so that the live and removed
    // predicates partition every row. The partial live-row index below depends on the live
    // half staying an equality test against 0.
    static constexpr std::string_view s_PUTT_IsRemoved = "is_removed"sv;
    static constexpr std::string_view s_PUTT_PackageRowId = "package_rowid"sv;
    static constexpr std::string_view s_PUTT_ChangeSequence = "change_seq"sv;
    static constexpr std::string_view s_PUTT_ChangeSequenceIndex_Name = "update_tracking_change_seq_idx"sv;

    namespace
    {
        // Finds the rowid that the package occupies in the index.
        std::optional<SQLite::rowid_t> GetPackageRowIdInIndex(const SQLite::Connection& connection, const std::string& packageIdentifier)
        {
            return V1_0::IdTable::SelectIdByValue(connection, packageIdentifier, true);
        }

        // The tombstones, as the identifier recorded for the package and the rowid it vacated.
        std::vector<std::pair<std::string, SQLite::rowid_t>> GetRemovedRows(
            const SQLite::Connection& connection,
            PackageUpdateTrackingTable::RemovalBehavior removals)
        {
            std::vector<std::pair<std::string, SQLite::rowid_t>> result;

            if (removals == PackageUpdateTrackingTable::RemovalBehavior::Delete)
            {
                return result;
            }

            Builder::StatementBuilder builder;
            builder.Select({ s_PUTT_Package, s_PUTT_PackageRowId }).From(s_PUTT_Table_Name).
                Where(s_PUTT_IsRemoved).NotEqualsLiteral(0);

            Statement statement = builder.Prepare(connection);

            while (statement.Step())
            {
                result.emplace_back(statement.GetColumn<std::string>(0), statement.GetColumn<SQLite::rowid_t>(1));
            }

            return result;
        }

        // The sequence to stamp on the row about to be written.
        int64_t GetNextChangeSequence(const SQLite::Connection& connection)
        {
            Builder::StatementBuilder builder;
            builder.Select().Column(Builder::Aggregate::Max, s_PUTT_ChangeSequence).From(s_PUTT_Table_Name);

            Statement statement = builder.Prepare(connection);

            // The aggregate produces a single row holding null when the table is empty.
            if (statement.Step() && !statement.GetColumnIsNull(0))
            {
                return statement.GetColumn<int64_t>(0) + 1;
            }

            return 1;
        }

        // The rows written after the given point, as measured by the given column.
        // The boundary is exclusive for the sequence and inclusive for the write time.
        std::vector<PackageUpdateTrackingTable::PackageData> GetUpdates(
            const SQLite::Connection& connection,
            std::string_view boundaryColumn,
            int64_t boundaryValue,
            bool exclusive,
            PackageUpdateTrackingTable::RemovalBehavior removals)
        {
            bool recordingRemovals = (removals == PackageUpdateTrackingTable::RemovalBehavior::Record);

            Builder::StatementBuilder builder;

            if (recordingRemovals)
            {
                builder.Select({ RowIDName, s_PUTT_Package, s_PUTT_WriteTime, s_PUTT_Manifest, s_PUTT_Hash, s_PUTT_PackageRowId });
            }
            else
            {
                builder.Select({ RowIDName, s_PUTT_Package, s_PUTT_WriteTime, s_PUTT_Manifest, s_PUTT_Hash });
            }

            builder.From(s_PUTT_Table_Name).Where(boundaryColumn);

            if (exclusive)
            {
                builder.IsGreaterThan(boundaryValue);
            }
            else
            {
                builder.IsGreaterThanOrEqualTo(boundaryValue);
            }

            if (recordingRemovals)
            {
                // Removals are reported separately, so that this remains the set of packages that
                // have data to write out, exactly as it is when removals delete their row.
                builder.And(s_PUTT_IsRemoved).Equals(0);
            }

            Statement select = builder.Prepare(connection);

            std::vector<PackageUpdateTrackingTable::PackageData> result;

            while (select.Step())
            {
                PackageUpdateTrackingTable::PackageData item;
                item.RowID = select.GetColumn<rowid_t>(0);
                item.PackageIdentifier = select.GetColumn<std::string>(1);
                item.WriteTime = select.GetColumn<int64_t>(2);
                item.Manifest = select.GetColumn<blob_t>(3);
                item.Hash = select.GetColumn<blob_t>(4);

                if (recordingRemovals)
                {
                    item.PackageRowId = select.GetColumn<rowid_t>(5);
                }

                result.emplace_back(std::move(item));
            }

            return result;
        }

        // The rowids vacated after the given point, as measured by the given column.
        std::set<SQLite::rowid_t> GetRemovals(
            const SQLite::Connection& connection,
            std::string_view boundaryColumn,
            int64_t boundaryValue,
            bool exclusive)
        {
            Builder::StatementBuilder builder;
            builder.Select(s_PUTT_PackageRowId).From(s_PUTT_Table_Name).Where(boundaryColumn);

            if (exclusive)
            {
                builder.IsGreaterThan(boundaryValue);
            }
            else
            {
                builder.IsGreaterThanOrEqualTo(boundaryValue);
            }

            builder.And(s_PUTT_IsRemoved).NotEqualsLiteral(0);

            Statement select = builder.Prepare(connection);

            std::set<SQLite::rowid_t> result;

            while (select.Step())
            {
                result.emplace(select.GetColumn<SQLite::rowid_t>(0));
            }

            return result;
        }
    }

    std::string_view PackageUpdateTrackingTable::TableName()
    {
        return s_PUTT_Table_Name;
    }

    void PackageUpdateTrackingTable::Create(SQLite::Connection& connection, RemovalBehavior removals)
    {
        using namespace Builder;

        bool recordingRemovals = (removals == RemovalBehavior::Record);

        StatementBuilder builder;
        builder.CreateTable(s_PUTT_Table_Name).BeginColumns();

        builder.Column(IntegerPrimaryKey());
        builder.Column(ColumnBuilder(s_PUTT_Package, Type::Text).NotNull());
        builder.Column(ColumnBuilder(s_PUTT_WriteTime, Type::Int64).NotNull());
        builder.Column(ColumnBuilder(s_PUTT_Manifest, Type::Blob).NotNull(!recordingRemovals));
        builder.Column(ColumnBuilder(s_PUTT_Hash, Type::Blob).NotNull(!recordingRemovals));

        if (recordingRemovals)
        {
            builder.Column(ColumnBuilder(s_PUTT_IsRemoved, Type::Int64).NotNull().Default(0));
            builder.Column(ColumnBuilder(s_PUTT_PackageRowId, Type::Int64).NotNull());
            builder.Column(ColumnBuilder(s_PUTT_ChangeSequence, Type::Int64).NotNull());
        }

        builder.EndColumns();

        builder.Execute(connection);

        StatementBuilder indexBuilder;
        indexBuilder.CreateIndex(s_PUTT_WriteTimeIndex_Name).On(s_PUTT_Table_Name).Columns(s_PUTT_WriteTime);
        indexBuilder.Execute(connection);

        if (recordingRemovals)
        {
            CreateLiveRowIndex(connection);
            CreateChangeSequenceIndex(connection);
        }
    }

    void PackageUpdateTrackingTable::CreateChangeSequenceIndex(SQLite::Connection& connection)
    {
        // Serves both the range scan that reports changes and the maximum that allocates the next
        // sequence. Not unique: a row updated in place takes a new sequence and leaves none behind,
        // but nothing depends on two rows never sharing one, and the migration backfills every
        // existing row with the same value.
        Builder::StatementBuilder builder;
        builder.CreateIndex(s_PUTT_ChangeSequenceIndex_Name).On(s_PUTT_Table_Name).Columns(s_PUTT_ChangeSequence);
        builder.Execute(connection);
    }

    void PackageUpdateTrackingTable::CreateLiveRowIndex(SQLite::Connection& connection)
    {
        // A package occupies exactly one rowid at a time, so at most one row per rowid can be
        // live. Tombstones are excluded because a rowid vacated by one package can be taken by
        // another, leaving the old package's tombstone and the new package's live row sharing it.
        //
        // The constraint is deliberately on the rowid rather than the identifier: a unique index
        // cannot use LIKE, and no available collation matches it. NOCASE is ASCII-only, while
        // LIKE here is the ICU implementation registered by sqlite3IcuInit, so NOCASE would
        // disagree with every other accessor on non-ASCII identifiers. An ICU collation cannot be
        // used either, since it would bake the ICU version into a published index file and make
        // the table unreadable to any connection that had not registered it.
        Builder::StatementBuilder builder;
        builder.CreateUniqueIndex(s_PUTT_LiveIndex_Name).On(s_PUTT_Table_Name).Columns(s_PUTT_PackageRowId).
            Where(s_PUTT_IsRemoved).EqualsLiteral(0);
        builder.Execute(connection);
    }

    void PackageUpdateTrackingTable::EnsureExists(SQLite::Connection& connection, RemovalBehavior removals)
    {
        if (!Exists(connection))
        {
            Create(connection, removals);
        }
    }

    void PackageUpdateTrackingTable::Drop(SQLite::Connection& connection)
    {
        Builder::StatementBuilder dropTableBuilder;
        dropTableBuilder.DropTable(s_PUTT_Table_Name);

        dropTableBuilder.Execute(connection);
    }

    bool PackageUpdateTrackingTable::Exists(const SQLite::Connection& connection)
    {
        Builder::StatementBuilder builder;
        builder.Select(Builder::RowCount).From(Builder::Schema::MainTable).
            Where(Builder::Schema::TypeColumn).Equals(Builder::Schema::Type_Table).And(Builder::Schema::NameColumn).Equals(s_PUTT_Table_Name);

        Statement statement = builder.Prepare(connection);
        THROW_HR_IF(E_UNEXPECTED, !statement.Step());
        return statement.GetColumn<int64_t>(0) != 0;
    }

    void PackageUpdateTrackingTable::Update(SQLite::Connection& connection, const ISQLiteIndex* internalIndex, const std::string& packageIdentifier, RemovalBehavior removals, bool ensureTable, std::optional<SQLite::rowid_t> removedPackageRowId)
    {
        if (ensureTable)
        {
            EnsureExists(connection, removals);
        }

        SearchRequest request;
        request.Inclusions.emplace_back(PackageMatchField::Id, MatchType::CaseInsensitive, packageIdentifier);
        auto result = internalIndex->Search(connection, request);

        if (result.Matches.empty())
        {
            if (removals == RemovalBehavior::Delete)
            {
                // Remove any existing package update row
                Builder::StatementBuilder deleteBuilder;
                deleteBuilder.DeleteFrom(s_PUTT_Table_Name).Where(s_PUTT_Package).LikeWithEscape(packageIdentifier);

                deleteBuilder.Execute(connection);
            }
            else
            {
                // Mark the package as removed rather than deleting the row, clearing the data columns.
                // The row is found by the rowid the package occupied rather than by its identifier.
                THROW_HR_IF(E_NOT_VALID_STATE, !removedPackageRowId);

                int64_t currentTime = Utility::GetCurrentUnixEpoch();
                int64_t changeSequence = GetNextChangeSequence(connection);

                Builder::StatementBuilder updateBuilder;
                updateBuilder.Update(s_PUTT_Table_Name).Set().
                    Column(s_PUTT_WriteTime).Equals(currentTime).
                    Column(s_PUTT_ChangeSequence).Equals(changeSequence).
                    Column(s_PUTT_Manifest).AssignValue(nullptr).
                    Column(s_PUTT_Hash).AssignValue(nullptr).
                    Column(s_PUTT_IsRemoved).Equals(1).
                    Where(s_PUTT_PackageRowId).Equals(removedPackageRowId.value()).
                    And(s_PUTT_IsRemoved).Equals(0);
                updateBuilder.Execute(connection);

                if (connection.GetChanges() == 0)
                {
                    // The package was added and removed without an intervening tracking checkpoint,
                    // so there is no row to mark. Record the removal so that a delta built against an
                    // older baseline still learns that the rowid was vacated.
                    Builder::StatementBuilder insertBuilder;
                    insertBuilder.InsertInto(s_PUTT_Table_Name).
                        Columns({ s_PUTT_Package, s_PUTT_WriteTime, s_PUTT_IsRemoved, s_PUTT_PackageRowId, s_PUTT_ChangeSequence }).
                        Values(packageIdentifier, currentTime, 1, removedPackageRowId.value(), changeSequence);
                    insertBuilder.Execute(connection);
                }
            }
        }
        else
        {
            THROW_HR_IF(E_UNEXPECTED, result.Matches.size() != 1);

            // Insert or update the package row
            std::vector<ISQLiteIndex::VersionKey> versionKeys = internalIndex->GetVersionKeysById(connection, result.Matches[0].first);

            Manifest::PackageVersionDataManifest manifest;

            for (const auto& key : versionKeys)
            {
                Manifest::PackageVersionDataManifest::VersionData versionData{
                    key.VersionAndChannel,
                    internalIndex->GetPropertyByPrimaryId(connection, key.ManifestId, PackageVersionProperty::ArpMinVersion),
                    internalIndex->GetPropertyByPrimaryId(connection, key.ManifestId, PackageVersionProperty::ArpMaxVersion),
                    internalIndex->GetPropertyByPrimaryId(connection, key.ManifestId, PackageVersionProperty::RelativePath),
                    internalIndex->GetPropertyByPrimaryId(connection, key.ManifestId, PackageVersionProperty::ManifestSHA256Hash)
                };

                manifest.AddVersion(std::move(versionData));
            }

            std::string manifestString = manifest.Serialize();

            auto compressor = Manifest::PackageVersionDataManifest::CreateCompressor();
            std::vector<uint8_t> compressedManifest = compressor.Compress(manifestString);

            Utility::SHA256::HashBuffer manifestHash = Utility::SHA256::ComputeHash(compressedManifest);
            int64_t currentTime = Utility::GetCurrentUnixEpoch();

            // The rowid the package occupies is the identity that a delta is keyed on, so it is
            // recorded alongside the data. It is resolved here rather than at removal time because
            // the package is still in the index at this point.
            SQLite::rowid_t packageRowId = 0;
            int64_t changeSequence = 0;

            if (removals == RemovalBehavior::Record)
            {
                std::optional<SQLite::rowid_t> indexRowId = GetPackageRowIdInIndex(connection, packageIdentifier);
                THROW_HR_IF(E_NOT_VALID_STATE, !indexRowId);
                packageRowId = indexRowId.value();
                changeSequence = GetNextChangeSequence(connection);
            }

            // First attempt to update the row and then insert it if no modification occurred.
            Builder::StatementBuilder updateBuilder;
            updateBuilder.Update(s_PUTT_Table_Name).Set().
                Column(s_PUTT_WriteTime).Equals(currentTime).
                Column(s_PUTT_Manifest).Equals(compressedManifest).
                Column(s_PUTT_Hash).Equals(manifestHash);

            if (removals == RemovalBehavior::Record)
            {
                // Clear the flag in case this package was previously removed and is now being re-added.
                updateBuilder.Column(s_PUTT_IsRemoved).Equals(0);
                updateBuilder.Column(s_PUTT_ChangeSequence).Equals(changeSequence);
            }

            updateBuilder.Where(s_PUTT_Package).LikeWithEscape(packageIdentifier);

            if (removals == RemovalBehavior::Record)
            {
                // Match on the rowid as well, so that a re-add only revives the tombstone that
                // refers to the rowid the package now occupies. A package removed and re-added
                // elsewhere leaves its old tombstone intact, which is what tells a delta to
                // suppress the baseline row it still holds. Restricting by rowid also prevents
                // this update from overwriting a different package's tombstone that happens to
                // name the same rowid.
                updateBuilder.And(s_PUTT_PackageRowId).Equals(packageRowId);
            }

            updateBuilder.Execute(connection);

            if (connection.GetChanges() == 0)
            {
                Builder::StatementBuilder insertBuilder;
                insertBuilder.InsertInto(s_PUTT_Table_Name).BeginColumns();

                insertBuilder.Column(s_PUTT_Package);
                insertBuilder.Column(s_PUTT_WriteTime);
                insertBuilder.Column(s_PUTT_Manifest);
                insertBuilder.Column(s_PUTT_Hash);

                if (removals == RemovalBehavior::Record)
                {
                    insertBuilder.Column(s_PUTT_PackageRowId);
                    insertBuilder.Column(s_PUTT_ChangeSequence);
                }

                insertBuilder.EndColumns().BeginValues();

                insertBuilder.Value(packageIdentifier);
                insertBuilder.Value(currentTime);
                insertBuilder.Value(compressedManifest);
                insertBuilder.Value(manifestHash);

                if (removals == RemovalBehavior::Record)
                {
                    insertBuilder.Value(packageRowId);
                    insertBuilder.Value(changeSequence);
                }

                insertBuilder.EndValues();

                insertBuilder.Execute(connection);
            }
        }
    }

    bool PackageUpdateTrackingTable::CheckConsistency(const SQLite::Connection& connection, ISQLiteIndex* internalIndex, RemovalBehavior removals, bool log)
    {
        bool result = true;

        // Ensure that all data in the update table matches the internal index
        for (const PackageData& packageData : GetUpdatesSince(connection, 0, removals))
        {
            auto manifestHash = Utility::SHA256::ComputeHash(packageData.Manifest);
            if (!Utility::SHA256::AreEqual(packageData.Hash, manifestHash))
            {
                if (!log)
                {
                    return false;
                }

                result = false;
                AICLI_LOG(Repo, Info, << "  [INVALID] value [" << s_PUTT_Hash << "] in table [" << s_PUTT_Table_Name <<
                    "] at row [" << packageData.RowID << "]; the hash of the manifest value does not match the hash in the row");
            }

            SearchRequest request;
            request.Inclusions.emplace_back(PackageMatchField::Id, MatchType::CaseInsensitive, packageData.PackageIdentifier);

            if (internalIndex->Search(connection, request).Matches.empty())
            {
                if (!log)
                {
                    return false;
                }

                result = false;
                AICLI_LOG(Repo, Info, << "  [INVALID] value [" << s_PUTT_Package << "] in table [" << s_PUTT_Table_Name <<
                    "] at row [" << packageData.RowID << "]; the package [" << packageData.PackageIdentifier << "] was not found in the internal index");
            }
        }

        // A package recorded as removed must no longer occupy the rowid it vacated.
        for (const auto& [packageIdentifier, vacatedRowId] : GetRemovedRows(connection, removals))
        {
            std::optional<SQLite::rowid_t> indexRowId = GetPackageRowIdInIndex(connection, packageIdentifier);

            if (indexRowId && indexRowId.value() == vacatedRowId)
            {
                if (!log)
                {
                    return false;
                }

                result = false;
                AICLI_LOG(Repo, Info, << "  [INVALID] value [" << s_PUTT_Package << "] in table [" << s_PUTT_Table_Name <<
                    "]; the package [" << packageIdentifier << "] is marked as having vacated rowid [" << vacatedRowId <<
                    "] but still occupies it in the internal index");
            }
        }

        // Every live row must name the rowid that the package actually occupies.
        if (removals == RemovalBehavior::Record)
        {
            for (const PackageData& packageData : GetUpdatesSince(connection, 0, removals))
            {
                std::optional<SQLite::rowid_t> indexRowId = GetPackageRowIdInIndex(connection, packageData.PackageIdentifier);

                if (!indexRowId || indexRowId.value() != packageData.PackageRowId)
                {
                    if (!log)
                    {
                        return false;
                    }

                    result = false;
                    AICLI_LOG(Repo, Info, << "  [INVALID] value [" << s_PUTT_PackageRowId << "] in table [" << s_PUTT_Table_Name <<
                        "] at row [" << packageData.RowID << "]; the package [" << packageData.PackageIdentifier << "] records rowid [" <<
                        packageData.PackageRowId << "] but occupies [" << (indexRowId ? std::to_string(indexRowId.value()) : "none") << "]");
                }
            }
        }

        // Ensure that all packages in the internal index are present in the update table
        Builder::StatementBuilder builder;
        builder.Select(Builder::RowCount).From(s_PUTT_Table_Name).Where(s_PUTT_Package).Like(Builder::Unbound).Escape(EscapeCharForLike);

        if (removals == RemovalBehavior::Record)
        {
            builder.And(s_PUTT_IsRemoved).Equals(0);
        }

        Statement select = builder.Prepare(connection);

        for (const auto& packageMatch : internalIndex->Search(connection, {}).Matches)
        {
            std::vector<ISQLiteIndex::VersionKey> versionKeys = internalIndex->GetVersionKeysById(connection, packageMatch.first);
            ISQLiteIndex::VersionKey& latestVersionKey = versionKeys[0];

            std::string packageIdentifier = internalIndex->GetPropertyByPrimaryId(connection, latestVersionKey.ManifestId, PackageVersionProperty::Id).value();

            select.Reset();
            select.Bind(1, packageIdentifier);
            select.Step();

            if (select.GetColumn<int64_t>(0) != 1)
            {
                if (!log)
                {
                    return false;
                }

                result = false;
                AICLI_LOG(Repo, Info, << "  [INVALID] value [" << packageIdentifier << "] in the internal index was not found as a non-removed entry in [" << s_PUTT_Table_Name << "]");
            }
        }

        return result;
    }

    std::vector<PackageUpdateTrackingTable::PackageData> PackageUpdateTrackingTable::GetUpdatesSince(const SQLite::Connection& connection, int64_t updateBaseTime, RemovalBehavior removals)
    {
        return GetUpdates(connection, s_PUTT_WriteTime, updateBaseTime, false, removals);
    }

    std::vector<PackageUpdateTrackingTable::PackageData> PackageUpdateTrackingTable::GetUpdatesSinceSequence(const SQLite::Connection& connection, int64_t baseSequence, RemovalBehavior removals)
    {
        // Only the recording form has the column, and only a delta asks this question.
        THROW_HR_IF(E_NOT_VALID_STATE, removals != RemovalBehavior::Record);

        return GetUpdates(connection, s_PUTT_ChangeSequence, baseSequence, true, removals);
    }

    std::set<SQLite::rowid_t> PackageUpdateTrackingTable::GetRemovalsSince(const SQLite::Connection& connection, int64_t updateBaseTime, RemovalBehavior removals)
    {
        if (removals == RemovalBehavior::Delete)
        {
            // Removals delete their row, so there is nothing to report.
            return {};
        }

        return GetRemovals(connection, s_PUTT_WriteTime, updateBaseTime, false);
    }

    std::set<SQLite::rowid_t> PackageUpdateTrackingTable::GetRemovalsSinceSequence(const SQLite::Connection& connection, int64_t baseSequence, RemovalBehavior removals)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, removals != RemovalBehavior::Record);

        return GetRemovals(connection, s_PUTT_ChangeSequence, baseSequence, true);
    }

    int64_t PackageUpdateTrackingTable::GetCurrentChangeSequence(const SQLite::Connection& connection, RemovalBehavior removals)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, removals != RemovalBehavior::Record);

        if (!Exists(connection))
        {
            // The table is created on demand, so an index to which no manifest has ever been
            // written has none. Nothing has been recorded, so nothing has been sequenced.
            return 0;
        }

        return GetNextChangeSequence(connection) - 1;
    }

    SQLite::blob_t PackageUpdateTrackingTable::GetDataHash(const SQLite::Connection& connection, const std::string& packageIdentifier, RemovalBehavior removals)
    {
        Builder::StatementBuilder builder;
        builder.Select(s_PUTT_Hash).From(s_PUTT_Table_Name).Where(s_PUTT_Package).LikeWithEscape(packageIdentifier);

        if (removals == RemovalBehavior::Record)
        {
            // A tombstone has its hash cleared, so restrict to the live row or this would be
            // liable to return a null hash for a package that was removed and re-added.
            builder.And(s_PUTT_IsRemoved).Equals(0);
        }

        Statement select = builder.Prepare(connection);

        THROW_HR_IF(E_NOT_SET, !select.Step());

        return select.GetColumn<SQLite::blob_t>(0);
    }

    void PackageUpdateTrackingTable::MigrateToRemovalTracking(SQLite::Connection& connection)
    {
        if (!Exists(connection))
        {
            return;
        }

        // The 2.1 shape differs from 2.0 in more than the columns it adds: a tombstone carries no
        // manifest or hash, so both have to become nullable, and SQLite cannot drop a not null
        // constraint in place.
        struct MigratedRow
        {
            rowid_t RowId = 0;
            std::string Package;
            int64_t WriteTime = 0;
            SQLite::blob_t Manifest;
            SQLite::blob_t Hash;
            rowid_t PackageRowId = 0;
        };

        std::vector<MigratedRow> rows;

        {
            Builder::StatementBuilder selectBuilder;
            selectBuilder.Select({ RowIDName, s_PUTT_Package, s_PUTT_WriteTime, s_PUTT_Manifest, s_PUTT_Hash }).From(s_PUTT_Table_Name);

            Statement select = selectBuilder.Prepare(connection);

            while (select.Step())
            {
                MigratedRow& row = rows.emplace_back();
                row.RowId = select.GetColumn<rowid_t>(0);
                row.Package = select.GetColumn<std::string>(1);
                row.WriteTime = select.GetColumn<int64_t>(2);
                row.Manifest = select.GetColumn<SQLite::blob_t>(3);
                row.Hash = select.GetColumn<SQLite::blob_t>(4);
            }
        }

        // Resolve the rowid for the rows already present. Every one of them is live: 2.0 deletes
        // the row when a package is removed, so a table being migrated has no tombstones and every
        // package it names is still in the index.
        for (MigratedRow& row : rows)
        {
            std::optional<SQLite::rowid_t> packageRowId = GetPackageRowIdInIndex(connection, row.Package);
            THROW_HR_IF(E_NOT_VALID_STATE, !packageRowId);
            row.PackageRowId = packageRowId.value();
        }

        // Dropping the table takes its indexes with it, so the rebuilt form is free to reuse their
        // names. The caller holds a savepoint, so the data is never at risk between the two.
        Drop(connection);
        Create(connection, RemovalBehavior::Record);

        Builder::StatementBuilder insertBuilder;
        insertBuilder.InsertInto(s_PUTT_Table_Name).
            Columns({ RowIDName, s_PUTT_Package, s_PUTT_WriteTime, s_PUTT_Manifest, s_PUTT_Hash, s_PUTT_PackageRowId, s_PUTT_IsRemoved, s_PUTT_ChangeSequence }).
            Values(Builder::Unbound, Builder::Unbound, Builder::Unbound, Builder::Unbound, Builder::Unbound, Builder::Unbound, 0, 0);

        Statement insert = insertBuilder.Prepare(connection);

        for (const MigratedRow& row : rows)
        {
            insert.Reset();
            insert.Bind(1, row.RowId);
            insert.Bind(2, row.Package);
            insert.Bind(3, row.WriteTime);
            insert.Bind(4, row.Manifest);
            insert.Bind(5, row.Hash);
            insert.Bind(6, row.PackageRowId);
            insert.Execute();
        }
    }
}
