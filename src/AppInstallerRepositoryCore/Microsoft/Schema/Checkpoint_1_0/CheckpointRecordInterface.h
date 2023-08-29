// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ICheckpointRecord.h"

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    struct CheckpointRecordInterface : public ICheckpointRecord
    {
        // Version 1.0
        Schema::Version GetVersion() const override;
        void CreateTables(SQLite::Connection& connection) override;

    private:
        bool IsEmpty(SQLite::Connection& connection) override;

        std::vector<std::string> GetAvailableCheckpoints(SQLite::Connection& connection) override;

        std::map<int, std::vector<std::string>> GetContextDataByContextId(SQLite::Connection& connection, std::string checkpointName, int64_t dataId) override;

        std::vector<int> GetAvailableDataTypes(SQLite::Connection& connection, std::string checkpointName) override;

        SQLite::rowid_t AddCheckpoint(SQLite::Connection& connection, std::string checkpointName) override;

        void AddContextData(SQLite::Connection& connection, std::string checkpointName, int dataId, std::string name, std::vector<std::string> values) override;





        //SQLite::rowid_t SetMetadata(SQLite::Connection& connection, std::string_view name, std::string_view value) override;
        //std::string GetMetadata(SQLite::Connection& connection, std::string_view name) override;
        //std::string GetLastCheckpoint(SQLite::Connection& connection) override;
        //bool CheckpointExists(SQLite::Connection& connection, std::string_view checkpointName) override;
        //std::vector<int> GetAvailableContextData(SQLite::Connection& connection, std::string_view checkpointName) override;
        //SQLite::rowid_t AddCheckpoint(SQLite::Connection& connection, std::string_view checkpointName) override;
        //SQLite::rowid_t AddContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData, std::string_view name, std::string_view value, int index = 0) override;
        //std::vector<std::string> GetContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData) override;
        //std::vector<std::string> GetContextDataByName(SQLite::Connection& connection, std::string_view checkpointName, int contextData, std::string_view name) override;
        //void RemoveContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData) override;
    };
}