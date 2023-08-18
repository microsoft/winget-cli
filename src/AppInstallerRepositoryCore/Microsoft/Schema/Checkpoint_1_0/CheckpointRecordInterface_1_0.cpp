// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointRecordInterface.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointContextTable.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointMetadataTable.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    Schema::Version CheckpointRecordInterface::GetVersion() const
    {
        return { 1, 0 };
    }

    void CheckpointRecordInterface::CreateTables(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createCheckpointTables_v1_0");
        Checkpoint_V1_0::CheckpointContextTable::Create(connection);
        Checkpoint_V1_0::CheckpointMetadataTable::Create(connection);
        Checkpoint_V1_0::CheckpointTable::Create(connection);
        savepoint.Commit();
    }

    SQLite::rowid_t CheckpointRecordInterface::SetMetadata(SQLite::Connection& connection, std::string_view name, std::string_view value)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "setMetadata_v1_0");
        SQLite::rowid_t argumentId = CheckpointMetadataTable::SetNamedValue(connection, name, value);
        savepoint.Commit();
        return argumentId;
    }
    
    std::string CheckpointRecordInterface::GetMetadata(SQLite::Connection& connection, std::string_view name)
    {
        return CheckpointMetadataTable::GetNamedValue(connection, name);
    }

    bool CheckpointRecordInterface::IsEmpty(SQLite::Connection& connection)
    {
        return CheckpointContextTable::IsEmpty(connection);
    }

    SQLite::rowid_t CheckpointRecordInterface::AddCheckpoint(SQLite::Connection& connection, std::string_view checkpointName)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addCheckpoint_v1_0");
        SQLite::rowid_t checkpointId = CheckpointTable::AddCheckpoint(connection, checkpointName);
        savepoint.Commit();
        return checkpointId;
    }

    std::optional<SQLite::rowid_t> CheckpointRecordInterface::GetCheckpointId(SQLite::Connection& connection, std::string_view checkpointName)
    {
        return CheckpointTable::GetCheckpointId(connection, checkpointName);
    }


    SQLite::rowid_t CheckpointRecordInterface::AddContextData(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int contextData, std::string_view name, std::string_view value, int index)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addContextData_v1_0");
        SQLite::rowid_t rowId = CheckpointContextTable::AddContextData(connection, checkpointId, contextData, name, value, index);
        savepoint.Commit();
        return rowId;
    }

    std::vector<std::string> CheckpointRecordInterface::GetContextData(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int contextData, std::string_view name)
    {
        return CheckpointContextTable::GetContextData(connection, checkpointId, contextData, name);
    }

    void CheckpointRecordInterface::RemoveContextData(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int contextData)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "removeContextData_v1_0");
        CheckpointContextTable::RemoveContextData(connection, checkpointId, contextData);
        savepoint.Commit();
    }
}