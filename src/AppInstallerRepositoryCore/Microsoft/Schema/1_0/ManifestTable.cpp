// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    using namespace std::string_view_literals;
    static constexpr std::string_view s_ManifestTable_Table_Name = "manifest"sv;

    void ManifestTable::Create(SQLite::Connection& connection, std::initializer_list<std::string_view> values)
    {
        std::ostringstream createTableSQL;
        createTableSQL << "CREATE TABLE [" << s_ManifestTable_Table_Name << "](";

        bool isFirst = true;
        for (const std::string_view& value : values)
        {
            createTableSQL << (isFirst ? "[" : ",[") << value << "] INT64";
            isFirst = false;
        }

        createTableSQL << ')';

        SQLite::Statement createStatement = SQLite::Statement::Create(connection, createTableSQL.str());

        createStatement.Execute();
    }
}
