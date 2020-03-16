// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_0/OneToManyTable.h"
#include "Microsoft/Schema/1_0/OneToOneTable.h"
#include "SQLiteStatementBuilder.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    namespace details
    {
        using namespace std::string_view_literals;
        static constexpr std::string_view s_OneToManyTable_MapTable_ManifestName = "manifest"sv;
        static constexpr std::string_view s_OneToManyTable_MapTable_Suffix = "_map"sv;
        static constexpr std::string_view s_OneToManyTable_MapTable_IndexSuffix = "_index"sv;

        namespace
        {
            // Create the mapping table insert statement for multiple use.
            // Bind the rowid of the value to 2.
            SQLite::Statement CreateMappingInsertStatementForManifestId(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, SQLite::rowid_t manifestId)
            {
                SQLite::Builder::StatementBuilder insertMappingBuilder;
                insertMappingBuilder.InsertInto({ tableName, s_OneToManyTable_MapTable_Suffix }).
                    Columns({ s_OneToManyTable_MapTable_ManifestName, valueName }).Values(manifestId, SQLite::Builder::Unbound);

                return insertMappingBuilder.Prepare(connection);
            }

            // Get a collection of the value ids associated with the given manifest id.
            std::vector<SQLite::rowid_t> GetValueIdsByManifestId(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, SQLite::rowid_t manifestId)
            {
                std::vector<SQLite::rowid_t> result;

                SQLite::Builder::StatementBuilder selectMappingBuilder;
                selectMappingBuilder.Select(valueName).From({ tableName, s_OneToManyTable_MapTable_Suffix }).Where(s_OneToManyTable_MapTable_ManifestName).Equals(manifestId);

                SQLite::Statement selectMappingStatement = selectMappingBuilder.Prepare(connection);

                while (selectMappingStatement.Step())
                {
                    result.push_back(selectMappingStatement.GetColumn<SQLite::rowid_t>(0));
                }

                return result;
            }

            struct DeleteValueIfNotNeededStatements
            {
                DeleteValueIfNotNeededStatements(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName)
                {
                    SQLite::Builder::StatementBuilder selectValueMappingBuilder;
                    selectValueMappingBuilder.Select(s_OneToManyTable_MapTable_ManifestName).From({ tableName, s_OneToManyTable_MapTable_Suffix }).Where(valueName).Equals(SQLite::Builder::Unbound).Limit(1);

                    SelectIfAnyMappingsByValueId = selectValueMappingBuilder.Prepare(connection);

                    SQLite::Builder::StatementBuilder deleteValueBuilder;
                    deleteValueBuilder.DeleteFrom(tableName).Where(SQLite::RowIDName).Equals(SQLite::Builder::Unbound);

                    DeleteValueById = deleteValueBuilder.Prepare(connection);
                }

                void Execute(SQLite::rowid_t valueId)
                {
                    SelectIfAnyMappingsByValueId.Reset();
                    SelectIfAnyMappingsByValueId.Bind(1, valueId);

                    // If no rows are found, we can delete the data.
                    if (!SelectIfAnyMappingsByValueId.Step())
                    {
                        DeleteValueById.Reset();
                        DeleteValueById.Bind(1, valueId);

                        DeleteValueById.Execute();
                    }
                }

            private:
                // Bind valid rowid to 1.
                SQLite::Statement SelectIfAnyMappingsByValueId;
                // Bind valid rowid to 1.
                SQLite::Statement DeleteValueById;
            };
        }

        std::string OneToManyTableGetMapTableName(std::string_view tableName)
        {
            std::string result(tableName);
            result += s_OneToManyTable_MapTable_Suffix;
            return result;
        }

        std::string_view OneToManyTableGetManifestColumnName()
        {
            return s_OneToManyTable_MapTable_ManifestName;
        }

        void CreateOneToManyTable(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName)
        {
            using namespace SQLite::Builder;

            SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ tableName } + "_create_v1_0");

            // Create the data table as a 1:1
            CreateOneToOneTable(connection, tableName, valueName);

            // Create the mapping table
            StatementBuilder createMapTableBuilder;
            createMapTableBuilder.CreateTable({ tableName, s_OneToManyTable_MapTable_Suffix }).Columns({
                ColumnBuilder(s_OneToManyTable_MapTable_ManifestName, Type::Int64).NotNull(),
                ColumnBuilder(valueName, Type::Int64).NotNull(),
                PrimaryKeyBuilder({ valueName, s_OneToManyTable_MapTable_ManifestName })
                });

            createMapTableBuilder.Execute(connection);

            StatementBuilder createMapTableIndexBuilder;
            createMapTableIndexBuilder.CreateIndex({ tableName, s_OneToManyTable_MapTable_Suffix, s_OneToManyTable_MapTable_IndexSuffix }).
                On({ tableName, s_OneToManyTable_MapTable_Suffix }).Columns(s_OneToManyTable_MapTable_ManifestName);

            createMapTableIndexBuilder.Execute(connection);

            savepoint.Commit();
        }

        void OneToManyTableEnsureExistsAndInsert(SQLite::Connection& connection,
            std::string_view tableName, std::string_view valueName,
            const std::vector<Utility::NormalizedString>& values, SQLite::rowid_t manifestId)
        {
            SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ tableName } + "_ensureandinsert_v1_0");

            SQLite::Statement insertMapping = CreateMappingInsertStatementForManifestId(connection, tableName, valueName, manifestId);

            for (const std::string& value : values)
            {
                // First, ensure that the data exists
                SQLite::rowid_t dataId = OneToOneTableEnsureExists(connection, tableName, valueName, value);

                // Second, insert into the mapping table
                insertMapping.Reset();
                insertMapping.Bind(2, dataId);

                insertMapping.Execute();
            }

            savepoint.Commit();
        }

        bool OneToManyTableUpdateIfNeededByManifestId(SQLite::Connection& connection,
            std::string_view tableName, std::string_view valueName,
            const std::vector<Utility::NormalizedString>& values, SQLite::rowid_t manifestId)
        {
            std::vector<SQLite::rowid_t> oldValueIds = GetValueIdsByManifestId(connection, tableName, valueName, manifestId);
            bool modificationNeeded = false;

            SQLite::Statement insertMapping = CreateMappingInsertStatementForManifestId(connection, tableName, valueName, manifestId);

            for (const std::string& value : values)
            {
                SQLite::rowid_t valueId = OneToOneTableEnsureExists(connection, tableName, valueName, value);

                auto itr = std::find(oldValueIds.begin(), oldValueIds.end(), valueId);
                if (itr != oldValueIds.end())
                {
                    oldValueIds.erase(itr);
                }
                else
                {
                    modificationNeeded = true;

                    insertMapping.Reset();
                    insertMapping.Bind(2, valueId);

                    insertMapping.Execute();
                }
            }

            // All incoming values are now present, we just need to delete the remaining old ones.
            SQLite::Builder::StatementBuilder deleteBuilder;
            deleteBuilder.DeleteFrom({ tableName, s_OneToManyTable_MapTable_Suffix }).
                Where(s_OneToManyTable_MapTable_ManifestName).Equals(manifestId).And(valueName).Equals(SQLite::Builder::Unbound);

            SQLite::Statement deleteStatement = deleteBuilder.Prepare(connection);

            DeleteValueIfNotNeededStatements dvinns(connection, tableName, valueName);

            for (SQLite::rowid_t valueId : oldValueIds)
            {
                modificationNeeded = true;

                // First, delete the mapping
                deleteStatement.Reset();
                deleteStatement.Bind(2, valueId);

                deleteStatement.Execute();

                // Second, delete the value itself if not needed
                dvinns.Execute(valueId);
            }

            return modificationNeeded;
        }

        void OneToManyTableDeleteIfNotNeededByManifestId(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, SQLite::rowid_t manifestId)
        {
            SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ tableName } + "_deleteifnotneeded_v1_0");

            // Get values referenced by the manifest id.
            std::vector<SQLite::rowid_t> values = GetValueIdsByManifestId(connection, tableName, valueName, manifestId);

            // Delete the mapping table rows with the manifest id.
            SQLite::Builder::StatementBuilder deleteBuilder;
            deleteBuilder.DeleteFrom({ tableName, s_OneToManyTable_MapTable_Suffix }).Where(s_OneToManyTable_MapTable_ManifestName).Equals(manifestId);

            deleteBuilder.Execute(connection);

            // For each value, see if any references exist
            DeleteValueIfNotNeededStatements dvinns(connection, tableName, valueName);

            for (SQLite::rowid_t value : values)
            {
                dvinns.Execute(value);
            }

            savepoint.Commit();
        }

        void OneToManyTablePrepareForPackaging(SQLite::Connection& connection, std::string_view tableName)
        {
            SQLite::Builder::StatementBuilder dropMapTableIndexBuilder;
            dropMapTableIndexBuilder.DropIndex({ tableName, s_OneToManyTable_MapTable_Suffix, s_OneToManyTable_MapTable_IndexSuffix });

            dropMapTableIndexBuilder.Execute(connection);
        }

        bool OneToManyTableIsEmpty(SQLite::Connection& connection, std::string_view tableName)
        {
            SQLite::Builder::StatementBuilder countBuilder;
            countBuilder.Select(SQLite::Builder::RowCount).From(tableName);

            SQLite::Statement countStatement = countBuilder.Prepare(connection);

            THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

            SQLite::Builder::StatementBuilder countMapBuilder;
            countMapBuilder.Select(SQLite::Builder::RowCount).From({ tableName, s_OneToManyTable_MapTable_Suffix });

            SQLite::Statement countMapStatement = countMapBuilder.Prepare(connection);

            THROW_HR_IF(E_UNEXPECTED, !countMapStatement.Step());

            return ((countStatement.GetColumn<int>(0) == 0) && (countMapStatement.GetColumn<int>(0) == 0));
        }
    }
}
