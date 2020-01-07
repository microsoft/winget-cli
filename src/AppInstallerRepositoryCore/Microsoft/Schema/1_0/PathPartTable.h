// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include <string_view>


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    // A table that represents a single manifest
    struct PathPartTable
    {
        // Creates the table.
        static void Create(SQLite::Connection& connection);

        // Gets the value name.
        static std::string_view ValueName();
    };
}
