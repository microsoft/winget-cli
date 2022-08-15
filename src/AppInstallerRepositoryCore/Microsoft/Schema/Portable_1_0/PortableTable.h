// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "SQLiteStatementBuilder.h"
#include <string_view>
#include <vector>

namespace AppInstaller::Repository::Microsoft::Schema::Portable_V1_0
{
    struct PortableTable
    {
        // Get the table name.
        static std::string_view TableName();

        // Creates the table with named indices.
        static void Create(SQLite::Connection& connection);

        // Insert the given values into the table.
        //static SQLite::rowid_t Insert(SQLite::Connection& connection, std::initializer_list<ManifestOneToOneValue> values);

        static bool ExistsById(const SQLite::Connection& connection, SQLite::rowid_t id);

        static void DeleteById(SQLite::Connection& connection, SQLite::rowid_t id);

        static bool IsEmpty(SQLite::Connection& connection);
    };
}
