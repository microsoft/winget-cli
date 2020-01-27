// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    using namespace std::string_view_literals;
    static constexpr std::string_view s_ManifestTable_Table_Name = "manifest"sv;

    void ManifestTable::Create(SQLite::Connection& connection, 
        std::initializer_list<std::string_view> valueNames,
        std::initializer_list<std::string_view> primaryKeyNames)
    {
        std::ostringstream createTableSQL;
        createTableSQL << "CREATE TABLE [" << s_ManifestTable_Table_Name << "] (";

        for (const std::string_view& valueName : valueNames)
        {
            createTableSQL << '[' << valueName << "] INT64 NOT NULL,";
        }

        createTableSQL << "PRIMARY KEY(";

        bool isFirst = true;
        for (const std::string_view& primaryKeyName : primaryKeyNames)
        {
            createTableSQL << (isFirst ? "[" : ", [") << primaryKeyName << "]";
            isFirst = false;
        }

        createTableSQL << "))";

        SQLite::Statement createStatement = SQLite::Statement::Create(connection, createTableSQL.str());

        createStatement.Execute();
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
