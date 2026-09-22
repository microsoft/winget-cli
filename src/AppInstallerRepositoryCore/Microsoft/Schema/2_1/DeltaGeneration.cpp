// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/2_1/DeltaGeneration.h"
#include "Microsoft/Schema/2_1/DeltaTables.h"
#include "Microsoft/Schema/2_1/Interface.h"

#include "Microsoft/Schema/2_0/PackagesTable.h"
#include "Microsoft/Schema/2_0/OneToManyTableWithMap.h"
#include "Microsoft/Schema/2_0/SystemReferenceStringTable.h"

#include <winget/SQLiteStatementBuilder.h>
#include <winget/SQLiteStorageBase.h>
#include <winget/SQLiteMetadataTable.h>


namespace AppInstaller::Repository::Microsoft::Schema::V2_1::Delta
{
    using namespace SQLite::Builder;

    namespace
    {
        struct DeltaDatabase : public SQLite::SQLiteStorageBase
        {
            DeltaDatabase(const std::filesystem::path& path, const SQLite::Version& version) :
                SQLiteStorageBase(path.u8string(), version)
            {
                SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "delta_create_database_v2_1");

                m_version.SetSchemaVersion(m_dbconn);
                CreateTables(m_dbconn);
                SetLastWriteTime();

                savepoint.Commit();
            }

            SQLite::Connection& GetConnection() { return m_dbconn; }
        };

        // Gets the rowid of the given package identifier in the packages table, if it is present.
        std::optional<SQLite::rowid_t> SelectPackageRowId(const SQLite::Connection& connection, const std::string& packageIdentifier)
        {
            StatementBuilder builder;
            builder.Select(SQLite::RowIDName).From(V2_0::PackagesTable::TableName()).
                Where(V2_0::PackagesTable::IdColumn::Name).LikeWithEscape(packageIdentifier);

            SQLite::Statement statement = builder.Prepare(connection);

            if (statement.Step())
            {
                return statement.GetColumn<SQLite::rowid_t>(0);
            }

            return {};
        }

        // Gets the identifier of the package at the given rowid, if there is one.
        std::optional<std::string> SelectPackageIdByRowId(const SQLite::Connection& connection, SQLite::rowid_t packageRowId)
        {
            StatementBuilder builder;
            builder.Select(V2_0::PackagesTable::IdColumn::Name).From(V2_0::PackagesTable::TableName()).
                Where(SQLite::RowIDName).Equals(packageRowId);

            SQLite::Statement statement = builder.Prepare(connection);

            if (statement.Step())
            {
                return statement.GetColumn<std::string>(0);
            }

            return {};
        }

        // Gets the rowid of the given value in a one to many data table, if it is present.
        std::optional<SQLite::rowid_t> SelectValueRowId(
            const SQLite::Connection& connection,
            std::string_view tableName,
            std::string_view valueName,
            const std::string& value)
        {
            StatementBuilder builder;
            builder.Select(SQLite::RowIDName).From(tableName).Where(valueName).Equals(value);

            SQLite::Statement statement = builder.Prepare(connection);

            if (statement.Step())
            {
                return statement.GetColumn<SQLite::rowid_t>(0);
            }

            return {};
        }

        // Gets the largest rowid in the given table, or 0 when it is empty.
        SQLite::rowid_t GetMaximumRowId(const SQLite::Connection& connection, std::string_view tableName)
        {
            StatementBuilder builder;
            builder.Select().Column(Aggregate::Max, SQLite::RowIDName).From(tableName);

            SQLite::Statement statement = builder.Prepare(connection);

            // The aggregate produces a single row holding null when the table is empty.
            if (statement.Step() && !statement.GetColumnIsNull(0))
            {
                return statement.GetColumn<SQLite::rowid_t>(0);
            }

            return 0;
        }

        // Determines the values that are in the first collection but not the second.
        std::vector<std::string> GetValuesNotIn(const std::vector<std::string>& values, const std::vector<std::string>& other)
        {
            std::vector<std::string> result;

            for (const std::string& value : values)
            {
                if (std::find(other.begin(), other.end(), value) == other.end())
                {
                    result.emplace_back(value);
                }
            }

            return result;
        }

        // Records that the package with the given identifier is no longer present.
        void WriteRemovedPackage(SQLite::Connection& deltaConnection, SQLite::rowid_t packageRowId, const std::string& packageIdentifier)
        {
            std::string tableName = GetTableName(V2_0::PackagesTable::TableName());

            StatementBuilder builder;
            builder.InsertInto(tableName).
                Columns({ SQLite::RowIDName, V2_0::PackagesTable::IdColumn::Name, IsRemovedColumnName() }).
                Values(packageRowId, packageIdentifier, 1);

            builder.Execute(deltaConnection);
        }

        // Copies the current state of a package into the delta.
        void WriteChangedPackage(SQLite::Connection& deltaConnection, const SQLite::Connection& sourceConnection, SQLite::rowid_t packageRowId)
        {
            auto [id, name, moniker, latestVersion, arpMinVersion, arpMaxVersion, hash] =
                V2_0::PackagesTable::GetValuesById<
                    V2_0::PackagesTable::IdColumn,
                    V2_0::PackagesTable::NameColumn,
                    V2_0::PackagesTable::MonikerColumn,
                    V2_0::PackagesTable::LatestVersionColumn,
                    V2_0::PackagesTable::ARPMinVersionColumn,
                    V2_0::PackagesTable::ARPMaxVersionColumn,
                    V2_0::PackagesTable::HashColumn
                >(sourceConnection, packageRowId);

            std::string tableName = GetTableName(V2_0::PackagesTable::TableName());

            StatementBuilder builder;
            builder.InsertInto(tableName).
                Columns({
                    SQLite::RowIDName,
                    V2_0::PackagesTable::IdColumn::Name,
                    V2_0::PackagesTable::NameColumn::Name,
                    V2_0::PackagesTable::MonikerColumn::Name,
                    V2_0::PackagesTable::LatestVersionColumn::Name,
                    V2_0::PackagesTable::ARPMinVersionColumn::Name,
                    V2_0::PackagesTable::ARPMaxVersionColumn::Name,
                    V2_0::PackagesTable::HashColumn::Name,
                    IsRemovedColumnName() }).
                Values(packageRowId, id, name, moniker, latestVersion, arpMinVersion, arpMaxVersion, hash, 0);

            builder.Execute(deltaConnection);
        }

        // Records a single system reference value as added or removed for a package.
        void WriteSystemReferenceValue(
            SQLite::Connection& deltaConnection,
            const ValueTableInfo& table,
            const std::string& value,
            SQLite::rowid_t packageRowId,
            bool isRemoved)
        {
            std::string tableName = GetTableName(table.TableName);

            StatementBuilder builder;
            builder.InsertInto(tableName).
                Columns({ table.ValueName, V2_0::details::SystemReferenceStringTableGetPrimaryColumnName(), IsRemovedColumnName() }).
                Values(value, packageRowId, isRemoved ? 1 : 0);

            builder.Execute(deltaConnection);
        }

        // Records only the system reference values that changed for the package.
        void WriteSystemReferenceDifference(
            SQLite::Connection& deltaConnection,
            const SQLite::Connection& sourceConnection,
            const SQLite::Connection& baselineConnection,
            const ValueTableInfo& table,
            SQLite::rowid_t packageRowId)
        {
            std::vector<std::string> currentValues = V2_0::details::SystemReferenceStringTableGetValuesByPrimaryId(
                sourceConnection, table.TableName, table.ValueName, packageRowId);
            std::vector<std::string> baselineValues = V2_0::details::SystemReferenceStringTableGetValuesByPrimaryId(
                baselineConnection, table.TableName, table.ValueName, packageRowId);

            for (const std::string& value : GetValuesNotIn(currentValues, baselineValues))
            {
                WriteSystemReferenceValue(deltaConnection, table, value, packageRowId, false);
            }

            for (const std::string& value : GetValuesNotIn(baselineValues, currentValues))
            {
                WriteSystemReferenceValue(deltaConnection, table, value, packageRowId, true);
            }
        }

        // Gets a rowid that identifies the value in the merged data table, creating one if needed.
        SQLite::rowid_t EnsureValueRowId(
            SQLite::Connection& deltaConnection,
            const SQLite::Connection& baselineConnection,
            const ValueTableInfo& table,
            const std::string& value,
            SQLite::rowid_t& nextValueRowId)
        {
            std::optional<SQLite::rowid_t> baselineRowId = SelectValueRowId(baselineConnection, table.TableName, table.ValueName, value);

            if (baselineRowId)
            {
                return baselineRowId.value();
            }

            std::string tableName = GetTableName(table.TableName);

            std::optional<SQLite::rowid_t> deltaRowId = SelectValueRowId(deltaConnection, tableName, table.ValueName, value);

            if (deltaRowId)
            {
                return deltaRowId.value();
            }

            SQLite::rowid_t newRowId = ++nextValueRowId;

            StatementBuilder builder;
            builder.InsertInto(tableName).
                Columns({ SQLite::RowIDName, table.ValueName }).
                Values(newRowId, value);

            builder.Execute(deltaConnection);

            return newRowId;
        }

        // Records a single map entry as added or removed for a package.
        void WriteOneToManyValue(
            SQLite::Connection& deltaConnection,
            const ValueTableInfo& table,
            SQLite::rowid_t valueRowId,
            SQLite::rowid_t packageRowId,
            bool isRemoved)
        {
            std::string mapTableName = GetMapTableName(table.TableName);

            StatementBuilder builder;
            builder.InsertInto(mapTableName).
                Columns({ table.ValueName, V2_0::details::OneToManyTableGetManifestColumnName(), IsRemovedColumnName() }).
                Values(valueRowId, packageRowId, isRemoved ? 1 : 0);

            builder.Execute(deltaConnection);
        }

        // Records only the map entries that changed for the package.
        void WriteOneToManyDifference(
            SQLite::Connection& deltaConnection,
            const SQLite::Connection& sourceConnection,
            const SQLite::Connection& baselineConnection,
            const ValueTableInfo& table,
            SQLite::rowid_t packageRowId,
            SQLite::rowid_t& nextValueRowId)
        {
            std::vector<std::string> currentValues = V2_0::details::OneToManyTableWithMapGetValuesByPrimaryId(
                sourceConnection, table.TableName, table.ValueName, packageRowId);
            std::vector<std::string> baselineValues = V2_0::details::OneToManyTableWithMapGetValuesByPrimaryId(
                baselineConnection, table.TableName, table.ValueName, packageRowId);

            for (const std::string& value : GetValuesNotIn(currentValues, baselineValues))
            {
                SQLite::rowid_t valueRowId = EnsureValueRowId(deltaConnection, baselineConnection, table, value, nextValueRowId);
                WriteOneToManyValue(deltaConnection, table, valueRowId, packageRowId, false);
            }

            for (const std::string& value : GetValuesNotIn(baselineValues, currentValues))
            {
                // The value came from the baseline's own data table, so it must be found there.
                std::optional<SQLite::rowid_t> valueRowId = SelectValueRowId(baselineConnection, table.TableName, table.ValueName, value);
                THROW_HR_IF(E_NOT_VALID_STATE, !valueRowId);

                WriteOneToManyValue(deltaConnection, table, valueRowId.value(), packageRowId, true);
            }
        }
    }

    void Generate(
        const SQLite::Connection& sourceConnection,
        const SQLite::Connection& baselineConnection,
        const std::filesystem::path& deltaOutputPath,
        const SQLite::Version& version,
        const std::vector<V2_0::PackageUpdateTrackingTable::PackageData>& changedPackages,
        const std::set<SQLite::rowid_t>& removedPackages)
    {
        AICLI_LOG(Repo, Info, << "Generating delta index at [" << deltaOutputPath << "] for " << changedPackages.size() <<
            " changed and " << removedPackages.size() << " removed packages");

        // A delta is only meaningful alongside the exact baseline it was computed from, so the
        // baseline has to be one that was designated as such and can therefore be named.
        std::optional<std::string> baselineIdentifier =
            SQLite::MetadataTable::TryGetNamedValue<std::string>(baselineConnection, s_MetadataValueName_BaselineIdentifier);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_INDEX_INTEGRITY_COMPROMISED, !baselineIdentifier || baselineIdentifier->empty());

        // The output is written in full before anything appears at the path that was asked for, so
        // that a failure partway through cannot leave a file that looks like a usable delta. An
        // existing file is refused rather than replaced: the database is opened for creation, so it
        // would otherwise be reopened and fail later with a confusing complaint about its tables.
        THROW_WIN32_IF(ERROR_FILE_EXISTS, std::filesystem::exists(deltaOutputPath));

        // A sibling of the destination, so that the two are necessarily on the same volume and
        // moving the result is a rename rather than a copy. The name carries a fresh GUID, so it
        // cannot collide with another generation running against the same destination directory.
        std::filesystem::path temporaryPath = deltaOutputPath.u8string() + "." + Utility::ConvertToUTF8(Utility::CreateNewGuidNameWString()) + ".tmp";
        THROW_WIN32_IF(ERROR_FILE_EXISTS, std::filesystem::exists(temporaryPath));

        auto removeTemporaryFilesOnExit = wil::scope_exit([&]()
            {
                try
                {
                    // The auxiliary files should be gone once the connection is closed, but a
                    // failure can leave them behind and they must not outlive the database.
                    for (const auto& suffix : { L"", L"-journal", L"-wal" })
                    {
                        std::filesystem::path auxiliaryPath = temporaryPath;
                        auxiliaryPath += suffix;
                        std::filesystem::remove(auxiliaryPath);
                    }
                }
                catch (...)
                {
                    AICLI_LOG(Repo, Info, << "Failed to remove temporary delta index file at: " << temporaryPath);
                }
            });

        {
            DeltaDatabase deltaDatabase{ temporaryPath, version };
            SQLite::Connection& deltaConnection = deltaDatabase.GetConnection();

            SQLite::Savepoint savepoint = SQLite::Savepoint::Create(deltaConnection, "delta_generate_v2_1");

            SQLite::MetadataTable::SetNamedValue(deltaConnection, s_MetadataValueName_DeltaBaselineIdentifier, baselineIdentifier.value());

            std::map<std::string_view, SQLite::rowid_t> nextValueRowIds;

            for (const auto& table : OneToManyTables())
            {
                nextValueRowIds[table.TableName] = GetMaximumRowId(baselineConnection, table.TableName);
            }

            // Every rowid already written to the delta's packages table. The changed packages are
            // written first so that a removal can tell whether the rowid it is about to vacate has
            // since been taken, and each removal joins the set so that two tombstones resolving to one
            // baseline rowid cannot both be written.
            std::set<SQLite::rowid_t> writtenRowIds;

            for (const auto& package : changedPackages)
            {
                // The rowid comes from the source rather than the baseline so that packages new to this
                // delta are covered by the same lookup; rowid stability is what makes the two agree.
                std::optional<SQLite::rowid_t> packageRowId = SelectPackageRowId(sourceConnection, package.PackageIdentifier);
                THROW_HR_IF(E_NOT_VALID_STATE, !packageRowId);

                AICLI_LOG(Repo, Verbose, << "Delta: recording change to [" << package.PackageIdentifier << "] (rowid " << packageRowId.value() << ")");

                writtenRowIds.insert(packageRowId.value());
                WriteChangedPackage(deltaConnection, sourceConnection, packageRowId.value());

                for (const auto& table : SystemReferenceTables())
                {
                    WriteSystemReferenceDifference(deltaConnection, sourceConnection, baselineConnection, table, packageRowId.value());
                }

                for (const auto& table : OneToManyTables())
                {
                    WriteOneToManyDifference(deltaConnection, sourceConnection, baselineConnection, table, packageRowId.value(), nextValueRowIds[table.TableName]);
                }
            }

            for (SQLite::rowid_t removedRowId : removedPackages)
            {
                // The rowid is resolved against the baseline directly, which is exact and is a primary
                // key lookup. Whatever identifier the tracking table recorded is irrelevant here: what
                // the delta suppresses is the baseline row at this rowid, so that row is also where the
                // identifier stored alongside the tombstone comes from.
                std::optional<std::string> baselinePackageId = SelectPackageIdByRowId(baselineConnection, removedRowId);

                if (!baselinePackageId)
                {
                    // The rowid was allocated after the baseline was produced, so as far as the
                    // baseline is concerned it never held anything and there is nothing to suppress.
                    AICLI_LOG(Repo, Verbose, << "Delta: rowid " << removedRowId << " was vacated but is not in the baseline");
                    continue;
                }

                if (writtenRowIds.count(removedRowId))
                {
                    // The rowid has already been written by a package that has since taken it.
                    AICLI_LOG(Repo, Verbose, << "Delta: rowid " << removedRowId << " was vacated but has already been written");
                    continue;
                }

                AICLI_LOG(Repo, Verbose, << "Delta: recording removal of [" << baselinePackageId.value() << "] (rowid " << removedRowId << ")");

                WriteRemovedPackage(deltaConnection, removedRowId, baselinePackageId.value());
            }

            savepoint.Commit();

            // Outside the savepoint, since this vacuums.
            PrepareTablesForPackaging(deltaConnection);
        }

        // Only now, with the database complete and its connection closed, does anything appear at
        // the path that was asked for.
        SQLite::SQLiteStorageBase::RenameSQLiteDatabase(temporaryPath, deltaOutputPath);
        removeTemporaryFilesOnExit.release();

        AICLI_LOG(Repo, Info, << "Delta index generation complete");
    }
}
