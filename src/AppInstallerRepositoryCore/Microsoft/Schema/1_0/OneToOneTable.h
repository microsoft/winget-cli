// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include <string_view>


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    namespace details
    {
        void CreateOneToOneTable(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName);
        SQLite::rowid_t OneToOneTableEnsureExists(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, std::string_view value);
    }

    // A table that represents a value that is 1:1 with a primary entry.
    template <typename TableInfo>
    struct OneToOneTable
    {
        // Creates the table.
        static void Create(SQLite::Connection& connection)
        {
            details::CreateOneToOneTable(connection, TableInfo::TableName(), TableInfo::ValueName());
        }

        static constexpr std::string_view ValueName()
        {
            return TableInfo::ValueName();
        }

        static SQLite::rowid_t EnsureExists(SQLite::Connection& connection, std::string_view value)
        {
            return details::OneToOneTableEnsureExists(connection, TableInfo::TableName(), TableInfo::ValueName(), value);
        }
    };
}
