// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/Version.h"

namespace AppInstaller::Repository::Microsoft::Schema
{
    struct ICheckpointIndex
    {
        virtual ~ICheckpointIndex() = default;

        // Gets the schema version that this index interface is built for.
        virtual Schema::Version GetVersion() const = 0;

        // Creates all of the version dependent tables within the database.
        virtual void CreateTables(SQLite::Connection& connection) = 0;

        // Version 1.0
        // Returns a bool value indicating whether all checkpoint tables are empty.
        virtual bool IsEmpty(SQLite::Connection& connection) = 0;

        // Sets the client version associated with this checkpoint index.
        virtual SQLite::rowid_t SetClientVersion(SQLite::Connection& connection, std::string_view clientVersion) = 0;

        // Gets the client version associated with this checkpoint index.
        virtual std::string GetClientVersion(SQLite::Connection& connection) = 0;

        // Sets the command name for a given context id.
        virtual SQLite::rowid_t SetCommandName(SQLite::Connection& connection, int contextId, std::string_view commandName) = 0;

        // Gets the command name for a given context id.
        virtual std::string GetCommandName(SQLite::Connection& connection, int contextId) = 0;

        // Adds a new row with the given context id to the checkpoint argument table.
        virtual SQLite::rowid_t AddContextToArgumentTable(SQLite::Connection& connection, int contextId) = 0;

        // removes the given context id from the checkpoint argument table.
        virtual void RemoveContextFromArgumentTable(SQLite::Connection& connection, int contextId) = 0;

        // Adds a new row with the given context id to the checkpoint context table.
        virtual SQLite::rowid_t AddContextToContextTable(SQLite::Connection& connection, int contextId) = 0;

        // Removes the given context id from the checkpoint context table.
        virtual void RemoveContextFromContextTable(SQLite::Connection& connection, int contextId) = 0;

        // Returns a boolean value indicating whether an argument exists for a given context id.
        virtual bool ContainsArgument(SQLite::Connection& connection, int contextId, std::string_view name) = 0;

        // Updates the boolean value of an argument for a given context id.
        virtual bool UpdateArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name, std::string_view value) = 0;

        // Updates the string value of an argument for a given context id.
        virtual bool UpdateArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name, bool value) = 0;

        // Gets the string value of an argument for a given context id.
        virtual std::string GetStringArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name) = 0;

        // Gets the boolean value of an argument for a given context id.
        virtual bool GetBoolArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name) = 0;

        // Gets the first context id in the checkpoint table.
        virtual int GetFirstContextId(SQLite::Connection& connection) = 0;

        // Sets the last checkpoint for a given context id.
        virtual bool SetLastCheckpointByContextId(SQLite::Connection& connection, int contextId, int checkpointFlag) = 0;

        // Gets the last checkpoint for a given context id.
        virtual int GetLastCheckpointByContextId(SQLite::Connection& connection, int contextId) = 0;
    };
}