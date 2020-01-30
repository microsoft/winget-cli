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
            SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ tableName } +"_create_v1_0");

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
            SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ tableName } +"_ensureandinsert_v1_0");

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

        void OneToManyTableDeleteIfNotNeededByManifestId(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, SQLite::rowid_t manifestId)
        {
            SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ tableName } +"_deleteifnotneeded_v1_0");

            // Get values referenced by the manifest id.
            std::vector<SQLite::rowid_t> values;

            std::ostringstream selectMappingSQL;
            selectMappingSQL << "SELECT [" << valueName << "] FROM [" << tableName << s_OneToManyTable_MapTable_Suffix << "] WHERE [" << s_OneToManyTable_MapTable_ManifestName << "] = ?";

            SQLite::Statement selectMappingStatement = SQLite::Statement::Create(connection, selectMappingSQL.str());

            selectMappingStatement.Bind(1, manifestId);

            while (selectMappingStatement.Step())
            {
                values.push_back(selectMappingStatement.GetColumn<SQLite::rowid_t>(0));
            }

            // Delete the mapping table rows with the manifest id.
            std::ostringstream deleteSQL;
            deleteSQL << "DELETE FROM [" << tableName << s_OneToManyTable_MapTable_Suffix << "] WHERE [" << s_OneToManyTable_MapTable_ManifestName << "] = ?";

            SQLite::Statement deleteStatement = SQLite::Statement::Create(connection, deleteSQL.str());

            deleteStatement.Bind(1, manifestId);

            deleteStatement.Execute();

            // For each value, see if any references exist
            std::ostringstream selectValueMappingSQL;
            selectValueMappingSQL << "SELECT [" << s_OneToManyTable_MapTable_ManifestName << "] "
                << "FROM [" << tableName << s_OneToManyTable_MapTable_Suffix << "] WHERE [" << valueName << "] = ? LIMIT 1";

            SQLite::Statement selectValueMappingStatement = SQLite::Statement::Create(connection, selectValueMappingSQL.str());

            std::ostringstream deleteValueSQL;
            deleteValueSQL << "DELETE FROM [" << tableName << "] WHERE [" << SQLite::RowIDName << "] = ?";

            SQLite::Statement deleteValueStatement = SQLite::Statement::Create(connection, deleteValueSQL.str());

            for (SQLite::rowid_t value : values)
            {
                selectValueMappingStatement.Reset();
                selectValueMappingStatement.Bind(1, value);

                // If no rows are found, we can delete the data.
                if (!selectValueMappingStatement.Step())
                {
                    deleteValueStatement.Reset();
                    deleteValueStatement.Bind(1, value);

                    deleteValueStatement.Execute();
                }
            }

            savepoint.Commit();
        }

        bool OneToManyTableIsEmpty(SQLite::Connection& connection, std::string_view tableName)
        {
            std::ostringstream countSQL;
            countSQL << "SELECT COUNT(*) FROM [" << tableName << ']';

            SQLite::Statement countStatement = SQLite::Statement::Create(connection, countSQL.str());

            THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

            std::ostringstream countMapSQL;
            countMapSQL << "SELECT COUNT(*) FROM [" << tableName << s_OneToManyTable_MapTable_Suffix << ']';

            SQLite::Statement countMapStatement = SQLite::Statement::Create(connection, countMapSQL.str());

            THROW_HR_IF(E_UNEXPECTED, !countMapStatement.Step());

            return ((countStatement.GetColumn<int>(0) == 0) && (countMapStatement.GetColumn<int>(0) == 0));
        }
    }
}
