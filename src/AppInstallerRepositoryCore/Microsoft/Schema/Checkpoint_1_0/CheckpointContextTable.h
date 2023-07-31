// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "SQLiteStatementBuilder.h"
#include "Microsoft/Schema/ICheckpointIndex.h"
#include <string_view>

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    struct CheckpointContextTable
    {
        // Get the table name.
        static std::string_view TableName();

        // Creates the table with named indices.
        static void Create(SQLite::Connection& connection);

        // Gets a value indicating whether the Checkpoint file with rowid id exists.
        static bool ExistsById(const SQLite::Connection& connection, SQLite::rowid_t id);

        // Deletes the Checkpoint row with the given rowid
        static void DeleteById(SQLite::Connection& connection, SQLite::rowid_t id);

        // Gets a value indicating whether the table is empty.
        static bool IsEmpty(SQLite::Connection& connection);

        // Selects the command argument by type from the table, returning the rowid if it exists.
        static std::optional<SQLite::rowid_t> SelectByArgumentType(const SQLite::Connection& connection, int type);

        // Adds a context row.
        static SQLite::rowid_t AddContext(SQLite::Connection& connection, int contextId);

        // Removes a context row.
        static void RemoveContext(SQLite::Connection& connection, int contextId);
    };
}