// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestTable.h"
#include "SQLiteStatementBuilder.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    using namespace std::string_view_literals;
    static constexpr std::string_view s_ManifestTable_Table_Name = "manifest"sv;

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
    }

    void ManifestTable::Create(SQLite::Connection& connection, std::initializer_list<ManifestColumnInfo> values)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createManifestTable_v1_0");

        std::ostringstream createTableSQL;
        createTableSQL << "CREATE TABLE [" << s_ManifestTable_Table_Name << "] (";

        for (const ManifestColumnInfo& value : values)
        {
            createTableSQL << '[' << value.Name << "] INT64 NOT NULL" << (value.Unique ? " UNIQUE" : "") << ",";
        }

        createTableSQL << "PRIMARY KEY(";

        bool isFirst = true;
        for (const ManifestColumnInfo& value : values)
        {
            if (value.PrimaryKey)
            {
                createTableSQL << (isFirst ? "[" : ", [") << value.Name << "]";
            }
            isFirst = false;
        }

        createTableSQL << "))";

        SQLite::Statement createStatement = SQLite::Statement::Create(connection, createTableSQL.str());

        createStatement.Execute();

        // Create an index on every value to improve performance
        for (const ManifestColumnInfo& value : values)
        {
            std::ostringstream createIndexSQL;
            createIndexSQL << "CREATE INDEX [" << s_ManifestTable_Table_Name << '_' << value.Name << "_index] "
                << "ON [" << s_ManifestTable_Table_Name << "]("
                << '[' << value.Name << "])";

            SQLite::Statement createIndex = SQLite::Statement::Create(connection, createIndexSQL.str());

            createIndex.Execute();
        }

        savepoint.Commit();
    }

    SQLite::rowid_t ManifestTable::Insert(SQLite::Connection& connection, std::initializer_list<ManifestOneToOneValue> values)
    {
        std::ostringstream insertSQL;
        insertSQL << "INSERT INTO [" << s_ManifestTable_Table_Name << "] (";

        bool isFirst = true;
        for (const ManifestOneToOneValue& value : values)
        {
            insertSQL << (isFirst ? "[" : ",[") << value.Name << "] ";
            isFirst = false;
        }

        insertSQL << ") VALUES (";

        for (size_t i = 0; i < values.size(); ++i)
        {
            insertSQL << (i == 0 ? "?" : ", ?");
        }

        insertSQL << ')';

        SQLite::Statement insert = SQLite::Statement::Create(connection, insertSQL.str());

        int bindIndex = 1;
        for (const ManifestOneToOneValue& value : values)
        {
            insert.Bind(bindIndex++, value.Value);
        }

        insert.Execute();

        return connection.GetLastInsertRowID();
    }

    void ManifestTable::DeleteById(SQLite::Connection& connection, SQLite::rowid_t id)
    {
        std::ostringstream deleteSQL;
        deleteSQL << "DELETE FROM [" << s_ManifestTable_Table_Name << "] WHERE [" << SQLite::RowIDName << "] = ?";

        SQLite::Statement deleteStatement = SQLite::Statement::Create(connection, deleteSQL.str());

        deleteStatement.Bind(1, id);

        deleteStatement.Execute();
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
