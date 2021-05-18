// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteStatementBuilder.h"

#include <string_view>

using namespace std::string_view_literals;


namespace AppInstaller::Repository::Microsoft::Schema::V1_3
{
    // A virtual table used to add a direct column onto the manifest table.
    struct HashVirtualTable
    {
        // The id type (which is actually the value for this virtual table)
        using id_t = SQLite::blob_t;

        // The name of the column.
        static constexpr std::string_view ValueName()
        {
            return "hash"sv;
        }

        // The name of the column.
        static constexpr SQLite::Builder::Type SQLiteType()
        {
            return SQLite::Builder::Type::Blob;
        }
    };
}
