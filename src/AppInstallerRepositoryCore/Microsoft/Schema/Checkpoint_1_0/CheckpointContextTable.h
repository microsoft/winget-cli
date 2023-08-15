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

        // Adds a checkpoint row.
        static SQLite::rowid_t AddCheckpoint(SQLite::Connection& connection, std::string_view checkpointName);

        static SQLite::rowid_t AddContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData, std::string_view name, std::string_view value);
        
        static std::string GetContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData, std::string_view name);

        static void RemoveContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData);
    };
}