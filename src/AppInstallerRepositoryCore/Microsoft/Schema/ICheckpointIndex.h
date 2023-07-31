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
        virtual SQLite::rowid_t SetClientVersion(SQLite::Connection& connection, std::string_view clientVersion) = 0;

        virtual std::string GetClientVersion(SQLite::Connection& connection) = 0;

        virtual SQLite::rowid_t SetCommandName(SQLite::Connection& connection, int contextId, std::string_view commandName) = 0;

        virtual std::string GetCommandName(SQLite::Connection& connection, int contextId) = 0;

        virtual bool IsEmpty(SQLite::Connection& connection) = 0;

        virtual SQLite::rowid_t AddContextToArgumentTable(SQLite::Connection& connection, int contextId) = 0;

        virtual SQLite::rowid_t AddContextToContextTable(SQLite::Connection& connection, int contextId) = 0;

        virtual void RemoveContextFromContextTable(SQLite::Connection& connection, int contextId) = 0;

        virtual void RemoveContextFromArgumentTable(SQLite::Connection& connection, int contextId) = 0;

        virtual bool UpdateArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name, std::string_view value) = 0;

        virtual bool UpdateArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name, bool value) = 0;

        virtual std::vector<std::string> GetAvailableArguments(SQLite::Connection& connection, int contextId) = 0;

        virtual bool ContainsArgument(SQLite::Connection& connection, int contextId, std::string_view name) = 0;

        virtual std::string GetStringArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name) = 0;

        virtual bool GetBoolArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name) = 0;

        virtual int GetFirstContextId(SQLite::Connection& connection) = 0;
    };
}