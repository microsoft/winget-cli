// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ICheckpointDatabase.h"

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    struct CheckpointDatabaseInterface : public ICheckpointDatabase
    {
        // Version 1.0
        SQLite::Version GetVersion() const override;
        void CreateTables(SQLite::Connection& connection) override;

    private:
        bool IsEmpty(SQLite::Connection& connection) override;
        SQLite::rowid_t AddCheckpoint(SQLite::Connection& connection, std::string_view checkpointName) override;
        std::vector<SQLite::rowid_t> GetCheckpointIds(SQLite::Connection& connection) override;
        std::vector<int> GetCheckpointDataTypes(SQLite::Connection& connection, SQLite::rowid_t checkpointId) override;
        std::vector<std::string> GetCheckpointDataFields(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType) override;
        void SetCheckpointDataValues(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType, std::string_view name, const std::vector<std::string>& values) override;
        std::optional<std::vector<std::string>> GetCheckpointDataFieldValues(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType, std::string_view name) override;
        void RemoveCheckpointDataType(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType) override;
    };
}