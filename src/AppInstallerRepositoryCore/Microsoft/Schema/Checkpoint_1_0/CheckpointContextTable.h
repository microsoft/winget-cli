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

        // Gets a value indicating whether a context with the given rowid exists.
        static bool ExistsById(const SQLite::Connection& connection, SQLite::rowid_t id);

        // Deletes the Checkpoint row with the given rowid
        static void DeleteById(SQLite::Connection& connection, SQLite::rowid_t id);

        // Gets a value indicating whether the table is empty.
        static bool IsEmpty(SQLite::Connection& connection);

        // Selects the context data by context id from the table, returning the rowid if it exists.
        static std::optional<SQLite::rowid_t> SelectByContextId(const SQLite::Connection& connection, int contextId);

        // Adds a context row.
        static SQLite::rowid_t AddContext(SQLite::Connection& connection, int contextId);

        // Removes a context row.
        static void RemoveContextById(SQLite::Connection& connection, SQLite::rowid_t id);
    
        // Set the last checkpoint for a given context id.
        static bool SetLastCheckpointById(SQLite::Connection& connection, SQLite::rowid_t id, int checkpointFlag);

        // Get the last checkpoint for a given context id.
        static int GetLastCheckpointById(SQLite::Connection& connection, SQLite::rowid_t id);

    };
}