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
        // Adds an argument to the index.
        virtual SQLite::rowid_t AddCommandArgument(SQLite::Connection& connection, int argumentType, const std::string_view& argumentValue) = 0;
        
        // Removes an argument from the index.
        //virtual SQLite::rowid_t RemoveCommandArgument(SQLite::Connection& connection, int argumentType, const std::string_view& argumentValue);
        
        virtual SQLite::rowid_t SetClientVersion(SQLite::Connection& connection, std::string_view clientVersion) = 0;

        virtual std::string GetClientVersion(SQLite::Connection& connection) = 0;

        virtual SQLite::rowid_t SetCommandName(SQLite::Connection& connection, std::string_view commandName) = 0;

        virtual std::string GetCommandName(SQLite::Connection& connection) = 0;

        virtual std::vector<std::pair<int, std::string>> GetArguments(SQLite::Connection& connection) = 0;

        // Returns a bool value indicating whether the index is empty.
        virtual bool IsEmpty(SQLite::Connection& connection) = 0;
    };
}