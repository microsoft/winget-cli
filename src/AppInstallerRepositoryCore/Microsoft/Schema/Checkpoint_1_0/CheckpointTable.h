// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "SQLiteStatementBuilder.h"
#include "Microsoft/Schema/ICheckpointRecord.h"
#include <string_view>

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    struct CheckpointTable
    {
        // Get the table name.
        static std::string_view TableName();

        // Creates the table with named indices.
        static void Create(SQLite::Connection& connection);

        // Adds a checkpoint.
        static SQLite::rowid_t AddCheckpoint(SQLite::Connection& connection, std::string_view checkpointName);

        // Gets the id of a checkpoint.
        static std::optional<SQLite::rowid_t> GetCheckpointId(SQLite::Connection& connection, std::string_view checkpointName);
    };
}