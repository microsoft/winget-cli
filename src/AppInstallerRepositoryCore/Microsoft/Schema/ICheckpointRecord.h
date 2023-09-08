// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/Version.h"

namespace AppInstaller::Repository::Microsoft::Schema
{
    struct ICheckpointRecord
    {
        virtual ~ICheckpointRecord() = default;

        // Gets the schema version that this index interface is built for.
        virtual Schema::Version GetVersion() const = 0;

        // Creates all of the version dependent tables within the database.
        virtual void CreateTables(SQLite::Connection& connection) = 0;

        // Version 1.0
        // Returns a bool value indicating whether all checkpoint tables are empty.
        virtual bool IsEmpty(SQLite::Connection& connection) = 0;

        virtual std::vector<std::string> GetAvailableCheckpoints(SQLite::Connection& connection) = 0;

        virtual SQLite::rowid_t AddCheckpoint(SQLite::Connection& connection, std::string_view checkpointName) = 0;

        virtual std::optional<SQLite::rowid_t> GetCheckpointIdByName(SQLite::Connection& connection, std::string_view checkpointName) = 0;

        virtual std::vector<int> GetCheckpointDataTypes(SQLite::Connection& connection, SQLite::rowid_t checkpointId) = 0;

        virtual std::string GetDataSingleValue(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType) = 0;

        virtual std::vector<std::string> GetCheckpointDataFields(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType) = 0;

        virtual std::vector<std::string> GetCheckpointDataValues(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType, std::string_view name) = 0;

        virtual bool HasCheckpointDataField(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType, std::string_view name) = 0;

        virtual void SetCheckpointDataValue(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType, std::string_view name, std::vector<std::string> values) = 0;
    };
}