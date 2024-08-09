// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestTable.h"
#include <winget/SQLiteStatementBuilder.h>
#include "OneToManyTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    using namespace std::string_view_literals;
    static constexpr std::string_view s_ManifestTable_Table_Name = "manifest"sv;
    static constexpr std::string_view s_ManifestTable_Index_Separator = "_"sv;
    static constexpr std::string_view s_ManifestTable_Index_Suffix = "_index"sv;

    namespace details
    {
        std::optional<SQLite::rowid_t> ManifestTableSelectByValueIds(
            const SQLite::Connection& connection,
            std::initializer_list<std::string_view> values,
            std::initializer_list<SQLite::rowid_t> ids)
        {
            THROW_HR_IF(E_INVALIDARG, values.size() != ids.size());

            SQLite::Builder::StatementBuilder builder;
            builder.Select(SQLite::RowIDName).From(s_ManifestTable_Table_Name);
            
            bool isFirst = true;

            for (const auto& value : values)
            {
                if (isFirst)
                {
                    builder.Where(value).Equals(SQLite::Builder::Unbound);
                    isFirst = false;
                }
                else
                {
                    builder.And(value).Equals(SQLite::Builder::Unbound);
                }
            }

            builder.Limit(1);

            SQLite::Statement select = builder.Prepare(connection);

            int bindIndex = 0;
            for (const auto& id : ids)
            {
                select.Bind(++bindIndex, id);
            }

            if (select.Step())
            {
                return select.GetColumn<SQLite::rowid_t>(0);
            }
            else
            {
                return {};
            }
        }

        SQLite::Statement ManifestTableGetIdsById_Statement(
            const SQLite::Connection& connection,
            SQLite::rowid_t id,
            std::initializer_list<std::string_view> values,
            bool stepAndVerify)
        {
            SQLite::Builder::StatementBuilder builder;
            builder.Select(values).From(s_ManifestTable_Table_Name).Where(SQLite::RowIDName).Equals(id);

            SQLite::Statement result = builder.Prepare(connection);

            if (stepAndVerify)
            {
                THROW_HR_IF(E_NOT_SET, !result.Step());
            }

            return result;
        }

        // Creates a statement and executes it, select the actual values for a given manifest id.
        // Ex.
        // SELECT [ids].[id] FROM [manifest]
        // JOIN [ids] ON [manifest].[id] = [ids].[rowid]
        // WHERE [manifest].[rowid] = 1
        SQLite::Statement ManifestTableGetValuesById_Statement(
            const SQLite::Connection& connection,
            SQLite::rowid_t id,
            std::initializer_list<SQLite::Builder::QualifiedColumn> columns,
            std::initializer_list<std::string_view> manifestColumnNames,
            bool stepAndVerify)
        {
            THROW_HR_IF(E_UNEXPECTED, manifestColumnNames.size() != columns.size());

            using QCol = SQLite::Builder::QualifiedColumn;

            SQLite::Builder::StatementBuilder builder;
            builder.Select(columns).From(s_ManifestTable_Table_Name);

            // join tables
            auto columnItr = columns.begin();
            auto manifestColumnNameItr = manifestColumnNames.begin();
            while (columnItr != columns.end())
            {
                builder.Join(columnItr->Table).On(QCol{ s_ManifestTable_Table_Name, *manifestColumnNameItr }, QCol{ columnItr->Table, SQLite::RowIDName });

                columnItr++;
                manifestColumnNameItr++;
            }

            builder.Where(QCol{ s_ManifestTable_Table_Name, SQLite::RowIDName }).Equals(id);

            SQLite::Statement result = builder.Prepare(connection);

            if (stepAndVerify)
            {
                THROW_HR_IF(E_NOT_SET, !result.Step());
            }

            return result;
        }

        SQLite::Statement ManifestTableGetAllValuesByIds_Statement(
            const SQLite::Connection& connection,
            std::initializer_list<SQLite::Builder::QualifiedColumn> valueColumns,
            std::initializer_list<SQLite::Builder::QualifiedColumn> joinColumns,
            std::initializer_list<std::string_view> idColumns,
            std::initializer_list<SQLite::rowid_t> ids)
        {
            using QCol = SQLite::Builder::QualifiedColumn;

            THROW_HR_IF(E_INVALIDARG, idColumns.size() != ids.size());

            SQLite::Builder::StatementBuilder builder;
            builder.Select(valueColumns).From(s_ManifestTable_Table_Name);

            for (const auto& joinColumn : joinColumns)
            {
                builder.Join(joinColumn.Table).On(QCol{ s_ManifestTable_Table_Name, joinColumn.Column }, QCol{ joinColumn.Table, SQLite::RowIDName });
            }

            bool isFirst = true;

            for (const auto& idColumn : idColumns)
            {
                if (isFirst)
                {
                    builder.Where(idColumn).Equals(SQLite::Builder::Unbound);
                    isFirst = false;
                }
                else
                {
                    builder.And(idColumn).Equals(SQLite::Builder::Unbound);
                }
            }

            SQLite::Statement select = builder.Prepare(connection);

            int bindIndex = 0;
            for (const auto& id : ids)
            {
                select.Bind(++bindIndex, id);
            }

            return select;
        }

        std::vector<std::pair<SQLite::rowid_t, std::string>> ManifestTableGetAllValuesByIds(
            const SQLite::Connection& connection,
            std::initializer_list<SQLite::Builder::QualifiedColumn> valueColumns,
            std::initializer_list<SQLite::Builder::QualifiedColumn> joinColumns,
            std::initializer_list<std::string_view> idColumns,
            std::initializer_list<SQLite::rowid_t> ids)
        {
            auto select = ManifestTableGetAllValuesByIds_Statement(connection, valueColumns, joinColumns, idColumns, ids);

            std::vector<std::pair<SQLite::rowid_t, std::string>> result;
            while (select.Step())
            {
                result.emplace_back(select.GetColumn<SQLite::rowid_t>(0), select.GetColumn<std::string>(1));
            }
            return result;
        }

        std::vector<int> ManifestTableBuildSearchStatement(
            SQLite::Builder::StatementBuilder& builder,
            std::initializer_list<SQLite::Builder::QualifiedColumn> columns,
            std::initializer_list<bool> isOneToOnes,
            std::string_view manifestAlias,
            std::string_view valueAlias,
            bool useLike)
        {
            using QCol = SQLite::Builder::QualifiedColumn;

            // Build a statement like:
            //      SELECT manifest.rowid as m, ids.id as v from manifest
            //      join ids on manifest.id = ids.rowid
            //      where ids.id = <value>
            // OR
            //      SELECT manifest.rowid as m, tags.tag as v from manifest
            //      join tags_map on manifest.rowid = tags_map.manifest
            //      join tags on tags_map.tag = tags.rowid
            //      where tags.tag = <value>
            // Where the joins and where portions are repeated for each table in question.
            builder.Select().
                Column(QCol(s_ManifestTable_Table_Name, SQLite::RowIDName)).As(manifestAlias);

            // Value will be captured for single tables references, and left empty for multi-tables
            if (columns.size() == 1)
            {
                builder.Column(*columns.begin());
            }
            else
            {
                builder.Value(std::string_view{});
            }

            builder.As(valueAlias).From(s_ManifestTable_Table_Name);

            // Create join clauses
            THROW_HR_IF(E_INVALIDARG, columns.size() != isOneToOnes.size());
            auto columnItr = columns.begin();
            auto isOneToOneItr = isOneToOnes.begin();

            for (; columnItr != columns.end(); ++columnItr, ++isOneToOneItr)
            {
                const SQLite::Builder::QualifiedColumn& column = *columnItr;

                if (*isOneToOneItr)
                {
                    builder.
                        Join(column.Table).On(QCol(s_ManifestTable_Table_Name, column.Column), QCol(column.Table, SQLite::RowIDName));
                }
                else
                {
                    std::string mapTableName = details::OneToManyTableGetMapTableName(column.Table);
                    builder.
                        Join(mapTableName).On(QCol(s_ManifestTable_Table_Name, SQLite::RowIDName), QCol(mapTableName, details::OneToManyTableGetManifestColumnName())).
                        Join(column.Table).On(QCol(mapTableName, column.Column), QCol(column.Table, SQLite::RowIDName));
                }
            }

            std::vector<int> result;

            // Create where clause
            for (const SQLite::Builder::QualifiedColumn& column : columns)
            {
                if (result.empty())
                {
                    builder.Where(column);
                }
                else
                {
                    builder.And(column);
                }

                if (useLike)
                {
                    builder.Like(SQLite::Builder::Unbound);
                    result.push_back(builder.GetLastBindIndex());
                    builder.Escape(SQLite::EscapeCharForLike);
                }
                else
                {
                    builder.Equals(SQLite::Builder::Unbound);
                    result.push_back(builder.GetLastBindIndex());
                }
            }

            return result;
        }

        SQLite::Statement ManifestTableUpdateValueIdById_Statement(SQLite::Connection& connection, std::string_view valueName)
        {
            SQLite::Builder::StatementBuilder builder;
            builder.Update(s_ManifestTable_Table_Name).Set().Column(valueName).Equals(SQLite::Builder::Unbound).Where(SQLite::RowIDName).Equals(SQLite::Builder::Unbound);

            return builder.Prepare(connection);
        }

        bool ManifestTableCheckConsistency(const SQLite::Connection& connection, const SQLite::Builder::QualifiedColumn& target, std::string_view manifestColumnName, bool log)
        {
            using QCol = SQLite::Builder::QualifiedColumn;

            // Build a select statement to find manifest rows containing references to 1:1 tables with nonexistent rowids
            // Such as:
            // Select manifest.rowid, manifest.id, ids.id from manifest left outer join ids on manifest.id = ids.rowid where ids.id is NULL
            SQLite::Builder::StatementBuilder builder;
            builder.
                Select({ QCol(s_ManifestTable_Table_Name, SQLite::RowIDName), QCol(s_ManifestTable_Table_Name, manifestColumnName) }).
                From(s_ManifestTable_Table_Name).
                LeftOuterJoin(target.Table).On(QCol(s_ManifestTable_Table_Name, manifestColumnName), QCol(target.Table, SQLite::RowIDName)).
                Where(target).IsNull();

            SQLite::Statement select = builder.Prepare(connection);
            bool result = true;

            while (select.Step())
            {
                result = false;

                if (!log)
                {
                    break;
                }

                AICLI_LOG(Repo, Info, << "  [INVALID] manifest [" << select.GetColumn<SQLite::rowid_t>(0) << "] refers to " << target.Table << " [" << select.GetColumn<SQLite::rowid_t>(1) << "]");
            }

            return result;
        }
    }

    std::string_view ManifestTable::TableName()
    {
        return s_ManifestTable_Table_Name;
    }

    // Starting in V1.1, all code should be going this route of creating named indices rather than using primary or unique keys on columns.
    // The resulting database will function the same, but give us control to drop the indices to reduce space.
    void ManifestTable::Create(SQLite::Connection& connection, std::initializer_list<ManifestColumnInfo> values)
    {
        using namespace SQLite::Builder;

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createManifestTable_v1_1");

        StatementBuilder createTableBuilder;
        createTableBuilder.CreateTable(s_ManifestTable_Table_Name).BeginColumns();

        // Add an integer primary key to keep the manifest rowid consistent
        createTableBuilder.Column(IntegerPrimaryKey());

        for (const ManifestColumnInfo& value : values)
        {
            createTableBuilder.Column(ColumnBuilder(value.Name, Type::Int64).NotNull());
        }

        createTableBuilder.EndColumns();

        createTableBuilder.Execute(connection);

        // Create a unique index with the primary key values
        StatementBuilder pkIndexBuilder;

        pkIndexBuilder.CreateUniqueIndex({ s_ManifestTable_Table_Name, s_ManifestTable_Index_Suffix }).On(s_ManifestTable_Table_Name).BeginColumns();

        for (const ManifestColumnInfo& value : values)
        {
            if (value.PrimaryKey)
            {
                pkIndexBuilder.Column(value.Name);
            }
        }

        pkIndexBuilder.EndColumns();

        pkIndexBuilder.Execute(connection);

        // Create an index on every value to improve performance
        for (const ManifestColumnInfo& value : values)
        {
            StatementBuilder createIndexBuilder;

            if (value.Unique)
            {
                createIndexBuilder.CreateUniqueIndex({ s_ManifestTable_Table_Name, s_ManifestTable_Index_Separator, value.Name, s_ManifestTable_Index_Suffix });
            }
            else
            {
                createIndexBuilder.CreateIndex({ s_ManifestTable_Table_Name, s_ManifestTable_Index_Separator, value.Name, s_ManifestTable_Index_Suffix });
            }

            createIndexBuilder.On(s_ManifestTable_Table_Name).Columns(value.Name);

            createIndexBuilder.Execute(connection);
        }

        savepoint.Commit();
    }

    void ManifestTable::AddColumn(SQLite::Connection& connection, AddedColumnInfo value)
    {
        using namespace SQLite::Builder;

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addColumnManifestTable_v1_3");

        StatementBuilder alterTableBuilder;
        alterTableBuilder.AlterTable(s_ManifestTable_Table_Name).Add(value.Name, value.Type);

        alterTableBuilder.Execute(connection);

        savepoint.Commit();
    }

    void ManifestTable::Create_deprecated(SQLite::Connection& connection, std::initializer_list<ManifestColumnInfo> values)
    {
        using namespace SQLite::Builder;

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createManifestTable_v1_0");

        StatementBuilder createTableBuilder;
        createTableBuilder.CreateTable(s_ManifestTable_Table_Name).BeginColumns();

        for (const ManifestColumnInfo& value : values)
        {
            createTableBuilder.Column(ColumnBuilder(value.Name, Type::Int64).NotNull().Unique(value.Unique));
        }

        PrimaryKeyBuilder pkBuilder;
        for (const ManifestColumnInfo& value : values)
        {
            if (value.PrimaryKey)
            {
                pkBuilder.Column(value.Name);
            }
        }

        createTableBuilder.Column(pkBuilder).EndColumns();

        createTableBuilder.Execute(connection);

        // Create an index on every value to improve performance
        for (const ManifestColumnInfo& value : values)
        {
            StatementBuilder createIndexBuilder;
            createIndexBuilder.CreateIndex({ s_ManifestTable_Table_Name, s_ManifestTable_Index_Separator, value.Name, s_ManifestTable_Index_Suffix }).
                On(s_ManifestTable_Table_Name).Columns(value.Name);

            createIndexBuilder.Execute(connection);
        }

        savepoint.Commit();
    }

    void ManifestTable::Drop(SQLite::Connection& connection)
    {
        SQLite::Builder::StatementBuilder dropTableBuilder;
        dropTableBuilder.DropTable(s_ManifestTable_Table_Name);

        dropTableBuilder.Execute(connection);
    }

    SQLite::rowid_t ManifestTable::Insert(SQLite::Connection& connection, std::initializer_list<ManifestOneToOneValue> values)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.InsertInto(s_ManifestTable_Table_Name).BeginColumns();

        for (const ManifestOneToOneValue& value : values)
        {
            builder.Column(value.Name);
        }

        builder.EndColumns().BeginValues();

        for (const ManifestOneToOneValue& value : values)
        {
            builder.Value(value.Value);
        }

        builder.EndValues();

        builder.Execute(connection);

        return connection.GetLastInsertRowID();
    }

    bool ManifestTable::ExistsById(const SQLite::Connection& connection, SQLite::rowid_t id)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::Builder::RowCount).From(s_ManifestTable_Table_Name).Where(SQLite::RowIDName).Equals(id);

        SQLite::Statement countStatement = builder.Prepare(connection);

        THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

        return (countStatement.GetColumn<int>(0) != 0);
    }

    void ManifestTable::DeleteById(SQLite::Connection& connection, SQLite::rowid_t id)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.DeleteFrom(s_ManifestTable_Table_Name).Where(SQLite::RowIDName).Equals(id);

        builder.Execute(connection);
    }

    void ManifestTable::PrepareForPackaging(SQLite::Connection& connection, std::initializer_list<std::string_view> values)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "pfpManifestTable_v1_1");

        PrepareForPackaging_deprecated(connection, values);

        SQLite::Builder::StatementBuilder dropPKIndexBuilder;
        dropPKIndexBuilder.DropIndex({ s_ManifestTable_Table_Name, s_ManifestTable_Index_Suffix });
        dropPKIndexBuilder.Execute(connection);

        savepoint.Commit();
    }

    void ManifestTable::PrepareForPackaging_deprecated(SQLite::Connection& connection, std::initializer_list<std::string_view> values)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "pfpManifestTable_v1_0");

        // Drop the index on the requested values
        for (std::string_view value : values)
        {
            SQLite::Builder::StatementBuilder dropIndexBuilder;
            dropIndexBuilder.DropIndex({ s_ManifestTable_Table_Name, s_ManifestTable_Index_Separator, value, s_ManifestTable_Index_Suffix });

            dropIndexBuilder.Execute(connection);
        }

        savepoint.Commit();
    }

    bool ManifestTable::IsValueReferenced(const SQLite::Connection& connection, std::string_view valueName, SQLite::rowid_t valueRowId)
    {
        return details::ManifestTableSelectByValueIds(connection, { valueName }, { valueRowId }).has_value();
    }

    bool ManifestTable::IsEmpty(SQLite::Connection& connection)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::Builder::RowCount).From(s_ManifestTable_Table_Name);

        SQLite::Statement countStatement = builder.Prepare(connection);

        THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

        return (countStatement.GetColumn<int>(0) == 0);
    }
}
