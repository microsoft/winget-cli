// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include <string_view>


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    namespace details
    {
        void CreateOneToManyTable(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName);
    }

    // A table that represents a value that is 1:N with a manifest.
    template <typename TableInfo>
    struct OneToManyTable
    {
        // Creates the table.
        static void Create(SQLite::Connection& connection)
        {
            details::CreateOneToManyTable(connection, TableInfo::TableName(), TableInfo::ValueName());
        }
    };
}
