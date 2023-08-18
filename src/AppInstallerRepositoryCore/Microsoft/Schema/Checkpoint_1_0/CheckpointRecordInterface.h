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
        SQLite::rowid_t SetMetadata(SQLite::Connection& connection, std::string_view name, std::string_view value) override;
        std::string GetMetadata(SQLite::Connection& connection, std::string_view name) override;

        SQLite::rowid_t AddCheckpoint(SQLite::Connection& connection, std::string_view checkpointName) override;
        std::optional<SQLite::rowid_t> GetCheckpointId(SQLite::Connection& connection, std::string_view checkpointName) override;
        SQLite::rowid_t AddContextData(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int contextData, std::string_view name, std::string_view value, int index = 0) override;
        
        std::vector<std::string> GetContextData(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int contextData, std::string_view name) override;
        void RemoveContextData(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int contextData) override;
    };
}