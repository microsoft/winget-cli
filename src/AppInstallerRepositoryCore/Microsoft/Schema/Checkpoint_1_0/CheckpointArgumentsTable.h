// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "SQLiteStatementBuilder.h"
#include "Microsoft/Schema/ICheckpointIndex.h"
#include <string_view>

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    struct CheckpointArgumentsTable
    {
        // Get the table name.
        static std::string_view TableName();

        // Creates the table with named indices.
        static void Create(SQLite::Connection& connection);

        // Gets a value indicating whether a context with the given id exists.
        static bool ExistsById(const SQLite::Connection& connection, SQLite::rowid_t id);

        // Deletes the context with the given id
        static void DeleteById(SQLite::Connection& connection, SQLite::rowid_t id);

        // Gets a value indicating whether the table is empty.
        static bool IsEmpty(SQLite::Connection& connection);

        // Selects the context arguments by context id from the table, returning the rowid if it exists.
        static std::optional<SQLite::rowid_t> SelectByContextId(const SQLite::Connection& connection, int contextId);

        // Adds a context with the given context id to the table.
        static SQLite::rowid_t AddContext(SQLite::Connection& connection, int contextId);

        // Removes a context from the table by id.
        static void RemoveContextById(SQLite::Connection& connection, SQLite::rowid_t id);

        // Gets a value indicating whether a context argument exists.
        static bool ContainsArgument(SQLite::Connection& connection, SQLite::rowid_t id, std::string_view name);

        // Updates a string argument by id.
        static bool UpdateArgumentById(SQLite::Connection& connection, SQLite::rowid_t id, std::string_view name, std::string_view value);
        
        // Gets a string value for an argument by id.
        static std::string GetStringArgumentById(SQLite::Connection& connection, SQLite::rowid_t id, std::string_view name);

        // Updates a bool argument by id.
        static bool UpdateArgumentById(SQLite::Connection& connection, SQLite::rowid_t id, std::string_view name, bool value);

        // Gets a boolean value for an argument by id.
        static bool GetBoolArgumentById(SQLite::Connection& connection, SQLite::rowid_t id, std::string_view name);

        // Sets the command name by id.
        static bool SetCommandNameById(SQLite::Connection& connection, SQLite::rowid_t id, std::string_view commandName);

        // Gets the command name by id.
        static std::string GetCommandNameById(SQLite::Connection& connection, SQLite::rowid_t id);

        // Gets the first context id from the table.
        static int GetFirstContextId(SQLite::Connection& connection);
    };
}