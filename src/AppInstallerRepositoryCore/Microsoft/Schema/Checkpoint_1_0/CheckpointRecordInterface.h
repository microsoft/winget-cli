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
        SQLite::rowid_t AddCheckpoint(SQLite::Connection& connection, std::string_view checkpointName) override;
        std::optional<SQLite::rowid_t> GetCheckpointIdByName(SQLite::Connection& connection, std::string_view checkpointName) override;
        std::vector<int> GetCheckpointDataTypes(SQLite::Connection& connection, SQLite::rowid_t checkpointId) override;
        std::vector<std::string> GetCheckpointDataFields(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType) override;
        std::vector<std::string> GetCheckpointDataValues(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType, std::string_view name) override;
        bool HasCheckpointDataField(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType, std::string_view name) override;
        void SetCheckpointDataValue(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType, std::string_view name, std::vector<std::string> values) override;



        std::string GetDataSingleValue(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType) override;
    };
}