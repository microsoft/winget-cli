// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteWrapper.h>
#include <vector>
#include <optional>

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    struct CheckpointTable
    {
        // Get the table name.
        static std::string_view TableName();

        // Creates the table with named indices.
        static void Create(SQLite::Connection& connection);

        // Returns all checkpoint ids in descending (newest at the front) order.
        static std::vector<SQLite::rowid_t> GetCheckpointIds(SQLite::Connection& connection);

        // Adds a checkpoint.
        static SQLite::rowid_t AddCheckpoint(SQLite::Connection& connection, std::string_view checkpointName);
    };
}