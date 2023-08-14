// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ICheckpointIndex.h"

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    struct CheckpointIndexInterface : public ICheckpointIndex
    {
        // Version 1.0
        Schema::Version GetVersion() const override;
        void CreateTables(SQLite::Connection& connection) override;

    private:
        bool IsEmpty(SQLite::Connection& connection) override;
        SQLite::rowid_t SetClientVersion(SQLite::Connection& connection, std::string_view clientVersion) override;
        std::string GetClientVersion(SQLite::Connection& connection) override;
        SQLite::rowid_t SetCommandName(SQLite::Connection& connection, std::string_view commandName) override;
        std::string GetCommandName(SQLite::Connection& connection) override;
        SQLite::rowid_t SetCommandArguments(SQLite::Connection& connection, std::string_view commandArguments) override;
        std::string GetCommandArguments(SQLite::Connection& connection) override;
        SQLite::rowid_t AddContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData, std::string_view name, std::string_view value) override;
        std::map<std::string, std::string> GetContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData) override;
        void RemoveContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData) override;

    };
}