// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "SQLiteStatementBuilder.h"
#include "Microsoft/Schema/ICheckpointRecord.h"
#include <string_view>

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    struct CheckpointContextTable
    {
        // Get the table name.
        static std::string_view TableName();

        // Creates the table with named indices.
        static void Create(SQLite::Connection& connection);

        // Gets a value indicating whether the table is empty.
        static bool IsEmpty(SQLite::Connection& connection);

        // Get all available context data.
        static std::vector<int> GetAvailableData(SQLite::Connection& connection, SQLite::rowid_t checkpointId);

        // Adds a context data for a checkpoint. Index is used to represent the item number if the context data has more than one value.
        static SQLite::rowid_t AddContextData(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int contextData, std::string_view name, std::string_view value, int index = 1);

        // Gets the context data values from a checkpoint id.
        static std::vector<std::string> GetContextData(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int contextData);

        // Gets the context data values by property name from a checkpoint id.
        static std::vector<std::string> GetContextDataByName(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int contextData, std::string_view name);

        // Removes the context data by checkpoint id.
        static void RemoveContextData(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int contextData);
    };
}