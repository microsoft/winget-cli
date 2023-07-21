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

        SQLite::rowid_t AddCommandArgument(SQLite::Connection& connection, int type, const std::string_view& argValue) override;

        SQLite::rowid_t SetClientVersion(SQLite::Connection& connection, std::string_view clientVersion) override;

        std::string GetClientVersion(SQLite::Connection& connection) override;

        SQLite::rowid_t SetCommandName(SQLite::Connection& connection, std::string_view clientVersion) override;

        std::string GetCommandName(SQLite::Connection& connection) override;

    private:
        bool IsEmpty(SQLite::Connection& connection) override;
    };
}