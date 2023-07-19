// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointIndexInterface.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    Schema::Version CheckpointIndexInterface::GetVersion() const
    {
        return { 1, 0 };
    }

    void CheckpointIndexInterface::CreateTable(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createCheckpointTable_v1_0");
        Checkpoint_V1_0::CheckpointTable::Create(connection);
        savepoint.Commit();
    }

    bool CheckpointIndexInterface::IsEmpty(SQLite::Connection& connection)
    {
        return CheckpointTable::IsEmpty(connection);
    }
}