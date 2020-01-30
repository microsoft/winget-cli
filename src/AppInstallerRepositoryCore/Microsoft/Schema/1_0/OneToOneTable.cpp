// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_0/OneToOneTable.h"
#include "Microsoft/Schema/1_0/ManifestTable.h"


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

        void OneToOneTableDeleteIfNotNeededById(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, SQLite::rowid_t id)
        {
            // If a manifest is found that references this id, then we are done.
            if (ManifestTableSelectByValueId(connection, valueName, id))
            {
                return;
            }

            std::ostringstream deleteSQL;
            deleteSQL << "DELETE FROM [" << tableName << "] WHERE [" << SQLite::RowIDName << "] = ?";

            SQLite::Statement deleteStatement = SQLite::Statement::Create(connection, deleteSQL.str());

            deleteStatement.Bind(1, id);

            deleteStatement.Execute();
        }

        bool OneToOneTableIsEmpty(SQLite::Connection& connection, std::string_view tableName)
        {
            std::ostringstream countSQL;
            countSQL << "SELECT COUNT(*) FROM [" << tableName << ']';

            SQLite::Statement countStatement = SQLite::Statement::Create(connection, countSQL.str());

            THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

            return (countStatement.GetColumn<int>(0) == 0);
        }
    }
}
