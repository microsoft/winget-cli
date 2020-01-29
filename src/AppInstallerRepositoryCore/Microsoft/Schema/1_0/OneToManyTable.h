// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include <string>
#include <string_view>
#include <vector>


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    namespace details
    {
        // Create the tables.
        void CreateOneToManyTable(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName);

        // Ensures that the value exists and inserts mapping entries.
        void OneToManyTableEnsureExistsAndInsert(SQLite::Connection& connection,
            std::string_view tableName, std::string_view valueName, 
            const std::vector<std::string>& values, SQLite::rowid_t manifestId);
    }

    // A table that represents a value that is 1:N with a primary entry.
    template <typename TableInfo>
    struct OneToManyTable
    {
        // Creates the table.
        static void Create(SQLite::Connection& connection)
        {
            details::CreateOneToManyTable(connection, TableInfo::TableName(), TableInfo::ValueName());
        }

        // Ensures that all values exist in the data table, and inserts into the mapping table for the given manifest id.
        static void EnsureExistsAndInsert(SQLite::Connection& connection, const std::vector<std::string>& values, SQLite::rowid_t manifestId)
        {
            details::OneToManyTableEnsureExistsAndInsert(connection, TableInfo::TableName(), TableInfo::ValueName(), values, manifestId);
        }
    };
}
