// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestTable.h"
#include "SQLiteStatementBuilder.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    using namespace std::string_view_literals;
    static constexpr std::string_view s_ManifestTable_Table_Name = "manifest"sv;
    static constexpr std::string_view s_ManifestTable_Index_Separator = "_"sv;
    static constexpr std::string_view s_ManifestTable_Index_Suffix = "_index"sv;

    namespace details
    {
        std::optional<SQLite::rowid_t> ManifestTableSelectByValueId(SQLite::Connection& connection, std::string_view valueName, SQLite::rowid_t id)
        {
            SQLite::Builder::StatementBuilder builder;
            builder.Select(SQLite::RowIDName).From(s_ManifestTable_Table_Name).Where(valueName).Equals(id).Limit(1);

            SQLite::Statement select = builder.Prepare(connection);

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
            SQLite::Connection& connection,
            SQLite::rowid_t id,
            std::initializer_list<std::string_view> values)
        {
            SQLite::Builder::StatementBuilder builder;
            builder.Select(values).From(s_ManifestTable_Table_Name).Where(SQLite::RowIDName).Equals(id);

            SQLite::Statement result = builder.Prepare(connection);

            THROW_HR_IF(E_NOT_SET, !result.Step());

            return result;
        }

        // Creates a statement and executes it, select the actual values for a given manifest id.
        // Ex.
        // SELECT [ids].[id] FROM [manifest]
        // JOIN [ids] ON [manifest].[id] = [ids].[rowid]
        // WHERE [manifest].[rowid] = 1
        SQLite::Statement ManifestTableGetValuesById_Statement(
            SQLite::Connection& connection,
            SQLite::rowid_t id,
            std::initializer_list<SQLite::Builder::QualifiedColumn> columns)
        {
            using QCol = SQLite::Builder::QualifiedColumn;

            SQLite::Builder::StatementBuilder builder;
            builder.Select(columns).From(s_ManifestTable_Table_Name);

            // join tables
            for (const QCol& column : columns)
            {
                builder.Join(column.Table).On(QCol{ s_ManifestTable_Table_Name, column.Column }, QCol{ column.Table, SQLite::RowIDName });
            }

            builder.Where(QCol{ s_ManifestTable_Table_Name, SQLite::RowIDName }).Equals(id);

            SQLite::Statement result = builder.Prepare(connection);

            THROW_HR_IF(E_NOT_SET, !result.Step());

            return result;
        }

        void ManifestTableUpdateValueIdById(SQLite::Connection& connection, std::string_view valueName, SQLite::rowid_t value, SQLite::rowid_t id)
        {
            SQLite::Builder::StatementBuilder builder;
            builder.Update(s_ManifestTable_Table_Name).Set().Column(valueName).Equals(value).Where(SQLite::RowIDName).Equals(id);

            builder.Execute(connection);
        }
    }

    void ManifestTable::Create(SQLite::Connection& connection, std::initializer_list<ManifestColumnInfo> values)
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

    void ManifestTable::DeleteById(SQLite::Connection& connection, SQLite::rowid_t id)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.DeleteFrom(s_ManifestTable_Table_Name).Where(SQLite::RowIDName).Equals(id);

        builder.Execute(connection);
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
