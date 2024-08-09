// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteWrapper.h>
#include <winget/SQLiteVersion.h>

namespace AppInstaller::Repository::Microsoft::Schema
{
    struct ICheckpointDatabase
    {
        virtual ~ICheckpointDatabase() = default;

        // Gets the schema version that this index interface is built for.
        virtual SQLite::Version GetVersion() const = 0;

        // Creates all of the version dependent tables within the database.
        virtual void CreateTables(SQLite::Connection& connection) = 0;

        // Version 1.0
        // Returns a bool value indicating whether all checkpoint tables are empty.
        virtual bool IsEmpty(SQLite::Connection& connection) = 0;

        // Returns all checkpoint ids in descending (newest at the front) order.
        virtual std::vector<SQLite::rowid_t> GetCheckpointIds(SQLite::Connection& connection) = 0;
         
        // Adds a new checkpoint to the Checkpoint table.
        virtual SQLite::rowid_t AddCheckpoint(SQLite::Connection& connection, std::string_view checkpointName) = 0;

        // Gets the data types associated with a checkpoint id.
        virtual std::vector<int> GetCheckpointDataTypes(SQLite::Connection& connection, SQLite::rowid_t checkpointId) = 0;

        // Sets the field values for a checkpoint data type.
        virtual void SetCheckpointDataValues(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType, std::string_view name, const std::vector<std::string>& values) = 0;

        // Gets all field names for a checkpoint data type.
        virtual std::vector<std::string> GetCheckpointDataFields(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType) = 0;

        // Gets all field values for a checkpoint data type.
        virtual std::optional<std::vector<std::string>> GetCheckpointDataFieldValues(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType, std::string_view name) = 0;
    
        // Removes all values for a checkpoint data type.
        virtual void RemoveCheckpointDataType(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType) = 0;
    };
}