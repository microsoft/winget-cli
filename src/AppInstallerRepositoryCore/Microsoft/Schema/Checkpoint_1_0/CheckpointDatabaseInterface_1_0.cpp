// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointDatabaseInterface.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointDataTable.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    SQLite::Version CheckpointDatabaseInterface::GetVersion() const
    {
        return { 1, 0 };
    }

    void CheckpointDatabaseInterface::CreateTables(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createCheckpointTables_v1_0");
        Checkpoint_V1_0::CheckpointTable::Create(connection);
        Checkpoint_V1_0::CheckpointDataTable::Create(connection);
        savepoint.Commit();
    }

    bool CheckpointDatabaseInterface::IsEmpty(SQLite::Connection& connection)
    {
        return CheckpointDataTable::IsEmpty(connection);
    }

    SQLite::rowid_t CheckpointDatabaseInterface::AddCheckpoint(SQLite::Connection& connection, std::string_view checkpointName)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addCheckpoint_v1_0");
        SQLite::rowid_t checkpointId = CheckpointTable::AddCheckpoint(connection, checkpointName);
        savepoint.Commit();
        return checkpointId;
    }

    std::vector<SQLite::rowid_t> CheckpointDatabaseInterface::GetCheckpointIds(SQLite::Connection& connection)
    {
        return CheckpointTable::GetCheckpointIds(connection);
    }

    std::vector<int> CheckpointDatabaseInterface::GetCheckpointDataTypes(SQLite::Connection& connection, SQLite::rowid_t checkpointId)
    {
        return CheckpointDataTable::GetAvailableData(connection, checkpointId);
    }

    std::vector<std::string> CheckpointDatabaseInterface::GetCheckpointDataFields(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType)
    {
        return CheckpointDataTable::GetDataFields(connection, checkpointId, dataType);
    }

    void CheckpointDatabaseInterface::SetCheckpointDataValues(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType, std::string_view name, const std::vector<std::string>& values)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "setCheckpointData_v1_0");

        if (values.empty())
        {
            CheckpointDataTable::AddCheckpointData(connection, checkpointId, dataType, name, {}, 0);
        }
        else
        {
            int index = 0;

            for (const auto& value : values)
            {
                CheckpointDataTable::AddCheckpointData(connection, checkpointId, dataType, name, value, index);
                index++;
            }
        }

        savepoint.Commit();
    }

    std::optional<std::vector<std::string>> CheckpointDatabaseInterface::GetCheckpointDataFieldValues(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType, std::string_view name)
    {
        return CheckpointDataTable::GetDataValuesByFieldName(connection, checkpointId, dataType, name);
    }

    void CheckpointDatabaseInterface::RemoveCheckpointDataType(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType)
    {
        CheckpointDataTable::RemoveDataType(connection, checkpointId, dataType);
    }
}