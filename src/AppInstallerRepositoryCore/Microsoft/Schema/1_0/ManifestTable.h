// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include <initializer_list>
#include <string_view>


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
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
        static void Create(SQLite::Connection& connection, 
            std::initializer_list<std::string_view> valueNames,
            std::initializer_list<std::string_view> primaryKeyNames);

        static SQLite::rowid_t Insert(SQLite::Connection& connection, std::initializer_list<ManifestOneToOneValue> values);
    };
}
