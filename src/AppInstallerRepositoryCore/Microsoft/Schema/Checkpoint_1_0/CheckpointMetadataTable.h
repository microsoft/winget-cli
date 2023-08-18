// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "SQLiteStatementBuilder.h"
#include "Microsoft/Schema/ICheckpointRecord.h"
#include <string_view>

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    struct CheckpointMetadataTable
    {
        // Get the table name.
        static std::string_view TableName();

        // Creates the table with named indices.
        static void Create(SQLite::Connection& connection);

        // Gets the metadata value of the checkpoint state.
        static std::string GetNamedValue(SQLite::Connection& connection, std::string_view name);

        // Sets the metadata value of the checkpoint state.
        static SQLite::rowid_t SetNamedValue(SQLite::Connection& connection, std::string_view name, std::string_view value);
    };
}