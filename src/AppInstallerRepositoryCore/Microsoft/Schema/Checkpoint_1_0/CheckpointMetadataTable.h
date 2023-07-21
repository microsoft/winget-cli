// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "SQLiteStatementBuilder.h"
#include "Microsoft/Schema/ICheckpointIndex.h"
#include <string_view>

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    struct CheckpointMetadataTable
    {
        // Get the table name.
        static std::string_view TableName();

        // Creates the table with named indices.
        static void Create(SQLite::Connection& connection);

        // Gets the client version of the saved state.
        static std::string GetClientVersion(SQLite::Connection& connection);

        // Sets the client version of the saved state.
        static SQLite::rowid_t SetClientVersion(SQLite::Connection& connection, std::string_view clientVersion);

        // Gets the command name of the saved state.
        static std::string GetCommandName(SQLite::Connection& connection);

        // Sets the command name of the saved state.
        static SQLite::rowid_t SetCommandName(SQLite::Connection& connection, std::string_view commandName);
    };
}