// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_0/OneToOneTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    namespace details
    {
        void CreateOneToOneTable(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName)
        {
            std::ostringstream createTableSQL;
            createTableSQL << "CREATE TABLE [" << tableName << "]("
                << '[' << valueName << "] TEXT NOT NULL PRIMARY KEY)";

            SQLite::Statement createStatement = SQLite::Statement::Create(connection, createTableSQL.str());

            createStatement.Execute();
        }

        SQLite::rowid_t OneToOneTableEnsureExists(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, std::string_view value)
        {
            {
                std::ostringstream selectSQL;
                selectSQL << "SELECT [" << SQLite::RowIDName << "] FROM [" << tableName << "] WHERE [" << valueName << "] = ?";

                SQLite::Statement select = SQLite::Statement::Create(connection, selectSQL.str());

                select.Bind(1, value);

                if (select.Step())
                {
                    return select.GetColumn<SQLite::rowid_t>(0);
                }
            }

            std::ostringstream insertSQL;
            insertSQL << "INSERT INTO [" << tableName << "] ([" << valueName << "]) VALUES (?)";

            SQLite::Statement insert = SQLite::Statement::Create(connection, insertSQL.str());

            insert.Bind(1, value);

            insert.Execute();

            return connection.GetLastInsertRowID();
        }
    }
}
