// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PackagesTable.h"
#include <winget/SQLiteStatementBuilder.h>
#include "OneToManyTableWithMap.h"


namespace AppInstaller::Repository::Microsoft::Schema::V2_0
{
    using namespace std::string_view_literals;
    static constexpr std::string_view s_PackagesTable_Table_Name = "packages"sv;
    static constexpr std::string_view s_PackagesTable_Index_Separator = "_"sv;
    static constexpr std::string_view s_PackagesTable_Index_Suffix = "_index"sv;

    namespace details
    {
        void PackagesTableCreate(SQLite::Connection& connection, std::initializer_list<ColumnInfo> values)
        {
            using namespace SQLite::Builder;

            SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createPackagesTable_v2_0");

            StatementBuilder createTableBuilder;
            createTableBuilder.CreateTable(s_PackagesTable_Table_Name).BeginColumns();

            // Add an integer primary key to keep the manifest rowid consistent
            createTableBuilder.Column(IntegerPrimaryKey());

            for (const ColumnInfo& value : values)
            {
                ColumnBuilder columnBuilder(value.Name, Type::Int64);

                if (!value.AllowNull)
                {
                    columnBuilder.NotNull();
                }

                createTableBuilder.Column(columnBuilder);
            }

            createTableBuilder.EndColumns();

            createTableBuilder.Execute(connection);

            // Create a unique index with the primary key values
            StatementBuilder pkIndexBuilder;

            pkIndexBuilder.CreateUniqueIndex({ s_PackagesTable_Table_Name, s_PackagesTable_Index_Suffix }).On(s_PackagesTable_Table_Name).BeginColumns();

            for (const ColumnInfo& value : values)
            {
                if (value.PrimaryKey)
                {
                    pkIndexBuilder.Column(value.Name);
                }
            }

            pkIndexBuilder.EndColumns();

            pkIndexBuilder.Execute(connection);

            // Create an index on every value to improve performance
            for (const ColumnInfo& value : values)
            {
                StatementBuilder createIndexBuilder;

                createIndexBuilder.CreateIndex({ s_PackagesTable_Table_Name, s_PackagesTable_Index_Separator, value.Name, s_PackagesTable_Index_Suffix });
                createIndexBuilder.On(s_PackagesTable_Table_Name).Columns(value.Name);

                createIndexBuilder.Execute(connection);
            }

            savepoint.Commit();
        }

        // Creates a statement and executes it, select the actual values for a given manifest id.
        // Ex.
        // SELECT [ids].[id] FROM [manifest]
        // JOIN [ids] ON [manifest].[id] = [ids].[rowid]
        // WHERE [manifest].[rowid] = 1
        SQLite::Statement PackagesTableGetValuesById_Statement(
            const SQLite::Connection& connection,
            SQLite::rowid_t id,
            std::initializer_list<std::string_view> columns,
            bool stepAndVerify)
        {
            SQLite::Builder::StatementBuilder builder;
            builder.Select(columns).From(s_PackagesTable_Table_Name).Where(SQLite::RowIDName).Equals(id);

            SQLite::Statement result = builder.Prepare(connection);

            if (stepAndVerify)
            {
                THROW_HR_IF(E_NOT_SET, !result.Step());
            }

            return result;
        }

        std::vector<int> PackagesTableBuildSearchStatement(
            SQLite::Builder::StatementBuilder& builder,
            std::initializer_list<std::string_view> columns,
            std::string_view primaryAlias,
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
                Column(QCol(s_PackagesTable_Table_Name, SQLite::RowIDName)).As(primaryAlias);

            // Value will be captured for single tables references, and left empty for multi-tables
            if (columns.size() == 1)
            {
                builder.Column(*columns.begin());
            }
            else
            {
                builder.LiteralColumn("");
            }

            builder.As(valueAlias).From(s_PackagesTable_Table_Name);

            std::vector<int> result;

            // Create where clause
            for (const auto& column : columns)
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

        SQLite::Statement PackagesTableUpdateValueIdById_Statement(SQLite::Connection& connection, std::string_view valueName)
        {
            SQLite::Builder::StatementBuilder builder;
            builder.Update(s_PackagesTable_Table_Name).Set().Column(valueName).Equals(SQLite::Builder::Unbound).Where(SQLite::RowIDName).Equals(SQLite::Builder::Unbound);

            return builder.Prepare(connection);
        }

        void PackagesTablePrepareForPackaging(SQLite::Connection& connection, std::initializer_list<ColumnInfo> values)
        {
            SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "pfpPackagesTable_v2_0");

            // Drop the index on the requested values
            for (const auto& value : values)
            {
                SQLite::Builder::StatementBuilder dropIndexBuilder;
                dropIndexBuilder.DropIndex({ s_PackagesTable_Table_Name, s_PackagesTable_Index_Separator, value.Name, s_PackagesTable_Index_Suffix });

                dropIndexBuilder.Execute(connection);
            }

            SQLite::Builder::StatementBuilder dropPKIndexBuilder;
            dropPKIndexBuilder.DropIndex({ s_PackagesTable_Table_Name, s_PackagesTable_Index_Suffix });
            dropPKIndexBuilder.Execute(connection);

            savepoint.Commit();
        }
    }

    std::string_view PackagesTable::TableName()
    {
        return s_PackagesTable_Table_Name;
    }

    void PackagesTable::Drop(SQLite::Connection& connection)
    {
        SQLite::Builder::StatementBuilder dropTableBuilder;
        dropTableBuilder.DropTable(s_PackagesTable_Table_Name);

        dropTableBuilder.Execute(connection);
    }

    bool PackagesTable::Exists(const SQLite::Connection& connection)
    {
        using namespace SQLite;

        Builder::StatementBuilder builder;
        builder.Select(Builder::RowCount).From(Builder::Schema::MainTable).
            Where(Builder::Schema::TypeColumn).Equals(Builder::Schema::Type_Table).And(Builder::Schema::NameColumn).Equals(s_PackagesTable_Table_Name);

        Statement statement = builder.Prepare(connection);
        THROW_HR_IF(E_UNEXPECTED, !statement.Step());
        return statement.GetColumn<int64_t>(0) != 0;
    }

    void PackagesTable::AddColumn(SQLite::Connection& connection, const ColumnInfo& value)
    {
        using namespace SQLite::Builder;

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addColumnPackagesTable_v2_0");

        StatementBuilder alterTableBuilder;
        alterTableBuilder.AlterTable(s_PackagesTable_Table_Name).Add(value.Name, value.Type);

        alterTableBuilder.Execute(connection);

        savepoint.Commit();
    }

    SQLite::rowid_t PackagesTable::Insert(SQLite::Connection& connection, const std::vector<NameValuePair>& values)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.InsertInto(s_PackagesTable_Table_Name).BeginColumns();

        for (const NameValuePair& value : values)
        {
            builder.Column(value.Name);
        }

        builder.EndColumns().BeginValues();

        for (const NameValuePair& value : values)
        {
            builder.Value(value.Value);
        }

        builder.EndValues();

        builder.Execute(connection);

        return connection.GetLastInsertRowID();
    }

    bool PackagesTable::ExistsById(const SQLite::Connection& connection, SQLite::rowid_t id)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::Builder::RowCount).From(s_PackagesTable_Table_Name).Where(SQLite::RowIDName).Equals(id);

        SQLite::Statement countStatement = builder.Prepare(connection);

        THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

        return (countStatement.GetColumn<int>(0) != 0);
    }

    std::vector<SQLite::rowid_t> PackagesTable::GetAllRowIds(const SQLite::Connection& connection, std::string_view orderByColumn, size_t limit)
    {
        SQLite::Builder::StatementBuilder selectBuilder;
        selectBuilder.Select(SQLite::RowIDName).From(s_PackagesTable_Table_Name).OrderBy(orderByColumn);

        if (limit)
        {
            selectBuilder.Limit(limit);
        }

        SQLite::Statement select = selectBuilder.Prepare(connection);

        std::vector<SQLite::rowid_t> result;
        while (select.Step())
        {
            result.emplace_back(select.GetColumn<SQLite::rowid_t>(0));
        }
        return result;
    }

    uint64_t PackagesTable::GetCount(const SQLite::Connection& connection)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::Builder::RowCount).From(s_PackagesTable_Table_Name);

        SQLite::Statement countStatement = builder.Prepare(connection);

        THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

        return static_cast<uint64_t>(countStatement.GetColumn<SQLite::rowid_t>(0));
    }

    bool PackagesTable::IsEmpty(SQLite::Connection& connection)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::Builder::RowCount).From(s_PackagesTable_Table_Name);

        SQLite::Statement countStatement = builder.Prepare(connection);

        THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

        return (countStatement.GetColumn<int>(0) == 0);
    }
}
