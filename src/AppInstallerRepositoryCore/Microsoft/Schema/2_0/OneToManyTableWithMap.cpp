// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/2_0/OneToManyTableWithMap.h"
#include "Microsoft/Schema/2_0/PackagesTable.h"
#include <winget/SQLiteStatementBuilder.h>


namespace AppInstaller::Repository::Microsoft::Schema::V2_0
{
    namespace details
    {
        using PrimaryTable = PackagesTable;

        using namespace std::string_view_literals;
        static constexpr std::string_view s_OneToManyTableWithMap_MapTable_PrimaryName = "package"sv;
        static constexpr std::string_view s_OneToManyTableWithMap_MapTable_Suffix = "_map"sv;
        static constexpr std::string_view s_OneToManyTableWithMap_MapTable_IndexSuffix = "_index"sv;
        static constexpr std::string_view s_OneToManyTableWithMap_PrimaryKeyIndexSuffix = "_pkindex"sv;

        namespace anon
        {
            // Create the mapping table insert statement for multiple use.
            // Bind the rowid of the value to 2.
            SQLite::Statement CreateMappingInsertStatementForPrimaryId(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, SQLite::rowid_t manifestId)
            {
                SQLite::Builder::StatementBuilder insertMappingBuilder;
                insertMappingBuilder.InsertOrIgnore({ tableName, s_OneToManyTableWithMap_MapTable_Suffix }).
                    Columns({ s_OneToManyTableWithMap_MapTable_PrimaryName, valueName }).Values(manifestId, SQLite::Builder::Unbound);

                return insertMappingBuilder.Prepare(connection);
            }

            // Get a collection of the value ids associated with the given primary id.
            std::vector<SQLite::rowid_t> GetValueIdsByPrimaryId(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, SQLite::rowid_t manifestId)
            {
                std::vector<SQLite::rowid_t> result;

                SQLite::Builder::StatementBuilder selectMappingBuilder;
                selectMappingBuilder.Select(valueName).From({ tableName, s_OneToManyTableWithMap_MapTable_Suffix }).Where(s_OneToManyTableWithMap_MapTable_PrimaryName).Equals(manifestId);

                SQLite::Statement selectMappingStatement = selectMappingBuilder.Prepare(connection);

                while (selectMappingStatement.Step())
                {
                    result.push_back(selectMappingStatement.GetColumn<SQLite::rowid_t>(0));
                }

                return result;
            }

            void CreateDataTable(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName)
            {
                using namespace SQLite::Builder;

                SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ tableName } + "_create_v2_0");

                StatementBuilder createTableBuilder;

                createTableBuilder.CreateTable(tableName).Columns({
                    IntegerPrimaryKey(),
                    ColumnBuilder(valueName, Type::Text).NotNull()
                    });

                createTableBuilder.Execute(connection);

                StatementBuilder indexBuilder;
                indexBuilder.CreateUniqueIndex({ tableName, s_OneToManyTableWithMap_PrimaryKeyIndexSuffix }).On(tableName).Columns(valueName);
                indexBuilder.Execute(connection);

                savepoint.Commit();
            }

            void DropDataTable(SQLite::Connection& connection, std::string_view tableName)
            {
                SQLite::Builder::StatementBuilder dropTableBuilder;
                dropTableBuilder.DropTable(tableName);

                dropTableBuilder.Execute(connection);
            }

            std::optional<SQLite::rowid_t> DataTableSelectIdByValue(const SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, std::string_view value, bool useLike)
            {
                SQLite::Builder::StatementBuilder selectBuilder;
                selectBuilder.Select(SQLite::RowIDName).From(tableName).Where(valueName);

                if (useLike)
                {
                    selectBuilder.LikeWithEscape(value);
                }
                else
                {
                    selectBuilder.Equals(value);
                }

                SQLite::Statement select = selectBuilder.Prepare(connection);

                if (select.Step())
                {
                    return select.GetColumn<SQLite::rowid_t>(0);
                }
                else
                {
                    return {};
                }
            }

            std::optional<std::string> DataTableSelectValueById(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, SQLite::rowid_t rowid)
            {
                SQLite::Builder::StatementBuilder selectBuilder;
                selectBuilder.Select(valueName).From(tableName).Where(SQLite::RowIDName).Equals(rowid);

                SQLite::Statement select = selectBuilder.Prepare(connection);

                if (select.Step())
                {
                    return select.GetColumn<std::string>(0);
                }
                else
                {
                    return {};
                }
            }

            SQLite::rowid_t DataTableEnsureExists(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, std::string_view value, bool overwriteLikeMatch = false)
            {
                auto selectResult = DataTableSelectIdByValue(connection, tableName, valueName, value, overwriteLikeMatch);
                if (selectResult)
                {
                    if (overwriteLikeMatch)
                    {
                        // If the value in the table is not an exact match, overwrite it with the incoming value
                        auto tableValue = DataTableSelectValueById(connection, tableName, valueName, selectResult.value());
                        if (tableValue.value() != value)
                        {
                            SQLite::Builder::StatementBuilder updateBuilder;
                            updateBuilder.Update(tableName).Set().Column(valueName).Equals(value).Where(SQLite::RowIDName).Equals(selectResult);

                            updateBuilder.Execute(connection);
                        }
                    }

                    return selectResult.value();
                }

                SQLite::Builder::StatementBuilder insertBuilder;
                insertBuilder.InsertInto(tableName).Columns(valueName).Values(value);

                insertBuilder.Execute(connection);

                return connection.GetLastInsertRowID();
            }

            void DataTablePrepareForPackaging(SQLite::Connection& connection, std::string_view tableName)
            {
                SQLite::Builder::StatementBuilder dropIndexBuilder;
                dropIndexBuilder.DropIndex({ tableName, s_OneToManyTableWithMap_PrimaryKeyIndexSuffix });
                dropIndexBuilder.Execute(connection);
            }

            bool DataTableCheckConsistency(const SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, bool log)
            {
                // Build a select statement to find values that contain an embedded null character
                // Such as:
                // Select count(*) from table where instr(value,char(0))>0
                SQLite::Builder::StatementBuilder builder;
                builder.
                    Select({ SQLite::RowIDName, valueName }).
                    From(tableName).
                    WhereValueContainsEmbeddedNullCharacter(valueName);

                SQLite::Statement select = builder.Prepare(connection);
                bool result = true;

                while (select.Step())
                {
                    result = false;

                    if (!log)
                    {
                        break;
                    }

                    AICLI_LOG(Repo, Info, << "  [INVALID] value in table [" << tableName << "] at row [" << select.GetColumn<SQLite::rowid_t>(0) << "] contains an embedded null character and starts with [" << select.GetColumn<std::string>(1) << "]");
                }

                return result;
            }
        }

        std::string OneToManyTableWithMapGetMapTableName(std::string_view tableName)
        {
            std::string result(tableName);
            result += s_OneToManyTableWithMap_MapTable_Suffix;
            return result;
        }

        std::string_view OneToManyTableWithMapGetManifestColumnName()
        {
            return s_OneToManyTableWithMap_MapTable_PrimaryName;
        }

        void CreateOneToManyTableWithMap(SQLite::Connection& connection, OneToManyTableSchema schemaVersion, std::string_view tableName, std::string_view valueName)
        {
            using namespace SQLite::Builder;

            SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ tableName } + "_create_v2_0");

            // Create the data table as a 1:1
            anon::CreateDataTable(connection, tableName, valueName);

            switch (schemaVersion)
            {
            case OneToManyTableSchema::Version_2_0:
            {
                // Create the mapping table
                StatementBuilder createMapTableBuilder;
                createMapTableBuilder.CreateTable({ tableName, s_OneToManyTableWithMap_MapTable_Suffix }).Columns({
                    ColumnBuilder(valueName, Type::Int64).NotNull(),
                    ColumnBuilder(s_OneToManyTableWithMap_MapTable_PrimaryName, Type::Int64).NotNull(),
                    PrimaryKeyBuilder({ valueName, s_OneToManyTableWithMap_MapTable_PrimaryName })
                    }).WithoutRowID();

                createMapTableBuilder.Execute(connection);
            }
                break;
            default:
                THROW_HR(E_UNEXPECTED);
            }

            StatementBuilder createMapTableIndexBuilder;
            createMapTableIndexBuilder.CreateIndex({ tableName, s_OneToManyTableWithMap_MapTable_Suffix, s_OneToManyTableWithMap_MapTable_IndexSuffix }).
                On({ tableName, s_OneToManyTableWithMap_MapTable_Suffix }).Columns({ s_OneToManyTableWithMap_MapTable_PrimaryName, valueName });

            createMapTableIndexBuilder.Execute(connection);

            savepoint.Commit();
        }

        void DropOneToManyTableWithMap(SQLite::Connection& connection, std::string_view tableName)
        {
            SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ tableName } + "_drop_v2_0");

            anon::DropDataTable(connection, tableName);

            SQLite::Builder::StatementBuilder dropTableBuilder;
            dropTableBuilder.DropTable({ tableName, s_OneToManyTableWithMap_MapTable_Suffix });

            dropTableBuilder.Execute(connection);

            savepoint.Commit();
        }

        std::vector<std::string> OneToManyTableWithMapGetValuesByPrimaryId(
            const SQLite::Connection& connection,
            std::string_view tableName,
            std::string_view valueName,
            SQLite::rowid_t manifestId)
        {
            using QCol = SQLite::Builder::QualifiedColumn;

            std::vector<std::string> result;

            SQLite::Builder::StatementBuilder builder;
            builder.Select(QCol(tableName, valueName)).
                From({ tableName, s_OneToManyTableWithMap_MapTable_Suffix }).As("map").Join(tableName).
                On(QCol("map", valueName), QCol(tableName, SQLite::RowIDName)).Where(QCol("map", s_OneToManyTableWithMap_MapTable_PrimaryName)).Equals(manifestId);

            SQLite::Statement statement = builder.Prepare(connection);

            while (statement.Step())
            {
                result.emplace_back(statement.GetColumn<std::string>(0));
            }

            return result;
        }

        void OneToManyTableWithMapEnsureExistsAndInsert(SQLite::Connection& connection,
            std::string_view tableName, std::string_view valueName,
            const std::vector<std::string>& values, SQLite::rowid_t manifestId)
        {
            SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ tableName } + "_ensureandinsert_v2_0");

            SQLite::Statement insertMapping = anon::CreateMappingInsertStatementForPrimaryId(connection, tableName, valueName, manifestId);

            for (const std::string& value : values)
            {
                // First, ensure that the data exists
                SQLite::rowid_t dataId = anon::DataTableEnsureExists(connection, tableName, valueName, value);

                // Second, insert into the mapping table
                insertMapping.Reset();
                insertMapping.Bind(2, dataId);

                insertMapping.Execute();
            }

            savepoint.Commit();
        }

        void OneToManyTableWithMapPrepareForPackaging(SQLite::Connection& connection, std::string_view tableName)
        {
            SQLite::Builder::StatementBuilder dropMapTableIndexBuilder;
            dropMapTableIndexBuilder.DropIndex({ tableName, s_OneToManyTableWithMap_MapTable_Suffix, s_OneToManyTableWithMap_MapTable_IndexSuffix });

            dropMapTableIndexBuilder.Execute(connection);

            anon::DataTablePrepareForPackaging(connection, tableName);
        }

        bool OneToManyTableWithMapCheckConsistency(const SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, bool log)
        {
            using QCol = SQLite::Builder::QualifiedColumn;
            constexpr std::string_view s_map = "map"sv;

            bool result = true;

            {
                // Build a select statement to find map rows containing references to primaries with nonexistent rowids
                // Such as:
                // Select map.rowid, map.primary from tags_map as map left outer join primary on map.primary = primary.rowid where primary.id is null

                SQLite::Builder::StatementBuilder builder;
                builder.
                    Select({ QCol(s_map, s_OneToManyTableWithMap_MapTable_PrimaryName), QCol(s_map, valueName) }).
                    From({ tableName, s_OneToManyTableWithMap_MapTable_Suffix }).As(s_map).
                    LeftOuterJoin(details::PrimaryTable::TableName()).On(QCol(s_map, s_OneToManyTableWithMap_MapTable_PrimaryName), QCol(details::PrimaryTable::TableName(), SQLite::RowIDName)).
                    Where(QCol(details::PrimaryTable::TableName(), SQLite::RowIDName)).IsNull();

                SQLite::Statement select = builder.Prepare(connection);

                while (select.Step())
                {
                    result = false;

                    if (!log)
                    {
                        break;
                    }

                    AICLI_LOG(Repo, Info, << "  [INVALID] " << tableName << s_OneToManyTableWithMap_MapTable_Suffix << " [" << select.GetColumn<SQLite::rowid_t>(0) <<
                        ", " << select.GetColumn<SQLite::rowid_t>(1) << "] refers to invalid " << details::PrimaryTable::TableName());
                }
            }

            if (!result && !log)
            {
                return result;
            }

            {
                // Build a select statement to find map rows containing references to 1:1 tables with nonexistent rowids
                // Such as:
                // Select map.rowid, map.tag from tags_map as map left outer join tags on map.tag = tags.rowid where tags.tag is null
                SQLite::Builder::StatementBuilder builder;
                builder.
                    Select({ QCol(s_map, s_OneToManyTableWithMap_MapTable_PrimaryName), QCol(s_map, valueName) }).
                    From({ tableName, s_OneToManyTableWithMap_MapTable_Suffix }).As(s_map).
                    LeftOuterJoin(tableName).On(QCol(s_map, valueName), QCol(tableName, SQLite::RowIDName)).
                    Where(QCol(tableName, valueName)).IsNull();

                SQLite::Statement select = builder.Prepare(connection);
                bool secondaryResult = true;

                while (select.Step())
                {
                    secondaryResult = false;

                    if (!log)
                    {
                        break;
                    }

                    AICLI_LOG(Repo, Info, << "  [INVALID] " << tableName << s_OneToManyTableWithMap_MapTable_Suffix << " [" << select.GetColumn<SQLite::rowid_t>(0) <<
                        ", " << select.GetColumn<SQLite::rowid_t>(1) << "] refers to invalid " << tableName);
                }

                result = result && secondaryResult;
            }

            if (!result && !log)
            {
                return result;
            }

            result = anon::DataTableCheckConsistency(connection, tableName, valueName, log) && result;

            return result;
        }

        bool OneToManyTableWithMapIsEmpty(SQLite::Connection& connection, std::string_view tableName)
        {
            SQLite::Builder::StatementBuilder countBuilder;
            countBuilder.Select(SQLite::Builder::RowCount).From(tableName);

            SQLite::Statement countStatement = countBuilder.Prepare(connection);

            THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

            SQLite::Builder::StatementBuilder countMapBuilder;
            countMapBuilder.Select(SQLite::Builder::RowCount).From({ tableName, s_OneToManyTableWithMap_MapTable_Suffix });

            SQLite::Statement countMapStatement = countMapBuilder.Prepare(connection);

            THROW_HR_IF(E_UNEXPECTED, !countMapStatement.Step());

            return ((countStatement.GetColumn<int>(0) == 0) && (countMapStatement.GetColumn<int>(0) == 0));
        }

        int OneToManyTableWithMapBuildSearchStatement(
            SQLite::Builder::StatementBuilder& builder,
            std::string_view tableName,
            std::string_view valueName,
            std::string_view primaryAlias,
            std::string_view valueAlias,
            bool useLike)
        {
            using QCol = SQLite::Builder::QualifiedColumn;
            constexpr std::string_view s_map = "map"sv;

            // Build a statement like:
            //      SELECT map.package as p, table.value as v from table
            //      join map on table.rowid = map.value
            //      where table.value = <value>
            builder.Select().
                Column(QCol(s_map, s_OneToManyTableWithMap_MapTable_PrimaryName)).As(primaryAlias).
                Column(QCol(tableName, valueName)).As(valueAlias).
                From(tableName).
                Join({ tableName, s_OneToManyTableWithMap_MapTable_Suffix }).As(s_map).On(QCol(tableName, SQLite::RowIDName), QCol(s_map, valueName)).
                Where(QCol(tableName, valueName));

            int result = -1;

            if (useLike)
            {
                builder.Like(SQLite::Builder::Unbound);
                result = builder.GetLastBindIndex();
                builder.Escape(SQLite::EscapeCharForLike);
            }
            else
            {
                builder.Equals(SQLite::Builder::Unbound);
                result = builder.GetLastBindIndex();
            }

            return result;
        }
    }
}
