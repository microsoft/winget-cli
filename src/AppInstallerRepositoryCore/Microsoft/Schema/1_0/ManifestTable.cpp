// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    using namespace std::string_view_literals;
    static constexpr std::string_view s_ManifestTable_Table_Name = "manifest"sv;

    namespace details
    {
        std::optional<SQLite::rowid_t> ManifestTableSelectByValueId(SQLite::Connection& connection, std::string_view valueName, SQLite::rowid_t id)
        {
            std::ostringstream selectSQL;
            selectSQL << "SELECT [" << SQLite::RowIDName << "] FROM [" << s_ManifestTable_Table_Name << "] WHERE [" << valueName << "] = ?";

            SQLite::Statement select = SQLite::Statement::Create(connection, selectSQL.str());

            select.Bind(1, id);

            if (select.Step())
            {
                return select.GetColumn<SQLite::rowid_t>(0);
            }
            else
            {
                return {};
            }
        }

        SQLite::Statement ManifestTableGetValuesById_Statement(
            SQLite::Connection& connection,
            std::initializer_list<std::string_view> tableNames,
            std::initializer_list<std::string_view> valueNames,
            SQLite::rowid_t id)
        {
            THROW_HR_IF(E_INVALIDARG, tableNames.size() != valueNames.size());

            std::ostringstream selectSQL;
            selectSQL << "SELECT ";

            // add columns to select
            tableNames

            SQLite::Statement result;
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
}
