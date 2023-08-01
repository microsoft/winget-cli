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
        SQLite::rowid_t SetCommandName(SQLite::Connection& connection, int contextId, std::string_view clientVersion) override;
        std::string GetCommandName(SQLite::Connection& connection, int contextId) override;
        SQLite::rowid_t AddContextToArgumentTable(SQLite::Connection& connection, int contextId) override;
        void RemoveContextFromArgumentTable(SQLite::Connection& connection, int contextId) override;
        SQLite::rowid_t AddContextToContextTable(SQLite::Connection& connection, int contextId) override;
        void RemoveContextFromContextTable(SQLite::Connection& connection, int contextId) override;
        bool ContainsArgument(SQLite::Connection& connection, int contextId, std::string_view name) override;
        bool UpdateArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name, std::string_view value) override;
        bool UpdateArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name, bool value) override;
        std::string GetStringArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name) override;
        bool GetBoolArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name) override;
        int GetFirstContextId(SQLite::Connection& connection) override;
        bool SetLastCheckpointByContextId(SQLite::Connection& connection, int contextId, int checkpointFlag) override;
        int GetLastCheckpointByContextId(SQLite::Connection& connection, int contextId) override;
    };
}