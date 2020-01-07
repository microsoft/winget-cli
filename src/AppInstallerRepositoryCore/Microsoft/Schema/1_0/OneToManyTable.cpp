// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_0/OneToManyTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    namespace details
    {
        static char const* const s_OneToManyTable_MapTable_ManifestName = "manifest";
        static char const* const s_OneToManyTable_MapTable_Suffix = "_map";

        void CreateOneToManyTable(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName)
        {
            SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ tableName } +"_create");

            // Create the table itself
            std::ostringstream createTableSQL;
            createTableSQL << "CREATE TABLE [" << tableName << "]("
                << '[' << valueName << "] TEXT PRIMARY KEY)";

            SQLite::Statement createTableStatement = SQLite::Statement::Create(connection, createTableSQL.str());

            createTableStatement.Execute();

            // Create the mapping table
            std::ostringstream createMapTableSQL;
            createMapTableSQL << "CREATE TABLE [" << tableName << s_OneToManyTable_MapTable_Suffix << "]("
                << "[" << s_OneToManyTable_MapTable_ManifestName << "] INT64 NOT NULL,"
                << '[' << valueName << "] INT64 NOT NULL,"
                "UNIQUE([" << s_OneToManyTable_MapTable_ManifestName << "], [" << valueName << "]))";

            SQLite::Statement createMapStatement = SQLite::Statement::Create(connection, createMapTableSQL.str());

            createMapStatement.Execute();

            savepoint.Commit();
        }
    }
}
