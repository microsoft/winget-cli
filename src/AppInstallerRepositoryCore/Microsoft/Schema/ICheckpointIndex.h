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

        // Sets the command name.
        virtual SQLite::rowid_t SetCommandName(SQLite::Connection& connection, std::string_view commandName) = 0;

        // Gets the command name.
        virtual std::string GetCommandName(SQLite::Connection& connection) = 0;

        // Sets the command arguments.
        virtual SQLite::rowid_t SetCommandArguments(SQLite::Connection& connection, std::string_view commandArguments) = 0;

        // Gets the command arguments.
        virtual std::string GetCommandArguments(SQLite::Connection& connection) = 0;

        // Adds the context data property for a given checkpoint.
        virtual SQLite::rowid_t AddContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData, std::string_view name, std::string_view value) = 0;

        // Removes the context data for a given checkpoint.
        virtual void RemoveContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData) = 0;
    };
}