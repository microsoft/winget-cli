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

        virtual std::map<std::string, std::vector<std::string>> GetContextDataByContextId(SQLite::Connection& connection, std::string checkpointName, int64_t dataId) = 0;

        virtual std::vector<int> GetAvailableDataTypes(SQLite::Connection& connection, std::string checkpointName) = 0;

        virtual SQLite::rowid_t AddCheckpoint(SQLite::Connection& connection, std::string checkpointName) = 0;

        virtual void AddContextData(SQLite::Connection& connection, std::string checkpointName, int dataId, std::string name, std::vector<std::string> values) = 0;

        // Sets the metadata value.
        //virtual SQLite::rowid_t SetMetadata(SQLite::Connection& connection, std::string_view name, std::string_view value) = 0;

        //// Gets the metadata value.
        //virtual std::string GetMetadata(SQLite::Connection& connection, std::string_view name) = 0;

        //// Gets the latest checkpoint.
        //virtual std::string GetLastCheckpoint(SQLite::Connection& connection) = 0;

        //// Returns a value indicating whether the checkpoint exists.
        //virtual bool CheckpointExists(SQLite::Connection& connection, std::string_view checkpointName) = 0;

        //// Gets the available context data from a checkpoint.
        //virtual std::vector<int> GetAvailableContextData(SQLite::Connection& connection, std::string_view checkpointName) = 0;

        //// Adds a checkpoint.
        //virtual SQLite::rowid_t AddCheckpoint(SQLite::Connection& connection, std::string_view checkpointName) = 0;

        //// Adds a context data value for a checkpoint.
        //virtual SQLite::rowid_t AddContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData, std::string_view name, std::string_view value, int index) = 0;

        //// Gets the context data values from a checkpoint.
        //virtual std::vector<std::string> GetContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData) = 0;

        //// Gets the context data values by property name from a checkpoint.
        //virtual std::vector<std::string> GetContextDataByName(SQLite::Connection& connection, std::string_view checkpointName, int contextData, std::string_view name) = 0;

        //// Removes the context data from a checkpoint.
        //virtual void RemoveContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData) = 0;
    };
}