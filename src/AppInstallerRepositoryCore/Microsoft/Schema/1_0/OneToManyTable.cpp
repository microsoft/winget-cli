// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_0/OneToManyTable.h"
#include "Microsoft/Schema/1_0/OneToOneTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    namespace details
    {
        using namespace std::string_view_literals;
        static constexpr std::string_view s_OneToManyTable_MapTable_ManifestName = "manifest"sv;
        static constexpr std::string_view s_OneToManyTable_MapTable_Suffix = "_map"sv;

        void CreateOneToManyTable(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName)
        {
            SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ tableName } +"_create");

            // Create the data table as a 1:1
            CreateOneToOneTable(connection, tableName, valueName);

            // Create the mapping table
            std::ostringstream createMapTableSQL;
            createMapTableSQL << "CREATE TABLE [" << tableName << s_OneToManyTable_MapTable_Suffix << "]("
                << "[" << s_OneToManyTable_MapTable_ManifestName << "] INT64 NOT NULL,"
                << '[' << valueName << "] INT64 NOT NULL,"
                "PRIMARY KEY([" << s_OneToManyTable_MapTable_ManifestName << "], [" << valueName << "]))";

            SQLite::Statement createMapStatement = SQLite::Statement::Create(connection, createMapTableSQL.str());

            createMapStatement.Execute();

            savepoint.Commit();
        }

        void OneToManyTableEnsureExistsAndInsert(SQLite::Connection& connection,
            std::string_view tableName, std::string_view valueName,
            const std::vector<std::string>& values, SQLite::rowid_t manifestId)
        {
            SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ tableName } +"_ensureandinsert");

            // Create the mapping table insert statement for multiple use
            std::ostringstream insertMappingSQL;
            insertMappingSQL << "INSERT INTO [" << tableName << s_OneToManyTable_MapTable_Suffix << "] ("
                << s_OneToManyTable_MapTable_ManifestName << ", " << valueName << ") VALUES (?, ?)";

            SQLite::Statement insertMapping = SQLite::Statement::Create(connection, insertMappingSQL.str());
            insertMapping.Bind(1, manifestId);

            for (const std::string& value : values)
            {
                // First, ensure that the data exists
                SQLite::rowid_t dataId = OneToOneTableEnsureExists(connection, tableName, valueName, value);

                // Second, insert into the mapping table
                insertMapping.Reset();
                insertMapping.Bind(2, dataId);

                insertMapping.Execute();
            }

            savepoint.Commit();
        }
    }
}
