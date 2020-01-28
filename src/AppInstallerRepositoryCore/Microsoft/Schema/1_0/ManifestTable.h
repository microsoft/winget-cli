// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include <initializer_list>
#include <optional>
#include <string_view>


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    namespace details
    {
        std::optional<SQLite::rowid_t> ManifestTableSelectByValueId(SQLite::Connection& connection, std::string_view valueName, SQLite::rowid_t id);
    }

    // Info on the manifest columns.
    struct ManifestColumnInfo
    {
        std::string_view Name;
        bool PrimaryKey;
        bool Unique;
    };

    // A value that is 1:1 with the manifest.
    struct ManifestOneToOneValue
    {
        std::string_view Name;
        SQLite::rowid_t Value;
    };

    // A table that represents a single manifest
    struct ManifestTable
    {
        // Creates the table.
        static void Create(SQLite::Connection& connection, std::initializer_list<ManifestColumnInfo> values);

        // Insert the given values into the table.
        static SQLite::rowid_t Insert(SQLite::Connection& connection, std::initializer_list<ManifestOneToOneValue> values);

        // Select the rowid of the manifest with the given value.
        template <typename Table>
        static std::optional<SQLite::rowid_t> SelectByValueId(SQLite::Connection& connection, SQLite::rowid_t id)
        {
            return details::ManifestTableSelectByValueId(connection, Table::ValueName(), id);
        }
    };
}
