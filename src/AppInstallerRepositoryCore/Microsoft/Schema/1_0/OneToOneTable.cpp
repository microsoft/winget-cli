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
                << '[' << valueName << "] TEXT PRIMARY KEY)";

            SQLite::Statement createStatement = SQLite::Statement::Create(connection, createTableSQL.str());

            createStatement.Execute();
        }
    }
}
