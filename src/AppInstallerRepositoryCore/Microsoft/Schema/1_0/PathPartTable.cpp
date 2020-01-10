// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PathPartTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    using namespace std::string_view_literals;
    static constexpr std::string_view s_PathPartTable_Table_Name = "pathparts"sv;
    static constexpr std::string_view s_PathPartTable_ParentValue_Name = "parent"sv;
    static constexpr std::string_view s_PathPartTable_PartValue_Name = "pathpart"sv;

    void PathPartTable::Create(SQLite::Connection& connection)
    {
        std::ostringstream createTableSQL;
        createTableSQL << "CREATE TABLE [" << s_PathPartTable_Table_Name << "]("
            << '[' << s_PathPartTable_ParentValue_Name << "] INT64,"
            << '[' << s_PathPartTable_PartValue_Name << "] TEXT)";

        SQLite::Statement createStatement = SQLite::Statement::Create(connection, createTableSQL.str());

        createStatement.Execute();
    }

    std::string_view PathPartTable::ValueName()
    {
        return s_PathPartTable_PartValue_Name;
    }
}
