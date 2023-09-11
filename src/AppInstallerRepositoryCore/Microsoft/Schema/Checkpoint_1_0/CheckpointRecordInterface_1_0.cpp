// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointRecordInterface.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointContextTable.h"
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
        Checkpoint_V1_0::CheckpointTable::Create(connection);
        Checkpoint_V1_0::CheckpointContextTable::Create(connection);
        savepoint.Commit();
    }

    bool CheckpointRecordInterface::IsEmpty(SQLite::Connection& connection)
    {
        return CheckpointContextTable::IsEmpty(connection);
    }

    std::vector<std::string> CheckpointRecordInterface::GetAvailableCheckpoints(SQLite::Connection& connection)
    {
        return CheckpointTable::GetCheckpoints(connection);
    }

    SQLite::rowid_t CheckpointRecordInterface::AddCheckpoint(SQLite::Connection& connection, std::string_view checkpointName)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addcheckpoint_v1_0");
        SQLite::rowid_t checkpointId = CheckpointTable::AddCheckpoint(connection, checkpointName);
        savepoint.Commit();
        return checkpointId;
    }

    std::optional<SQLite::rowid_t> CheckpointRecordInterface::GetCheckpointIdByName(SQLite::Connection& connection, std::string_view checkpointName)
    {
        auto result = CheckpointTable::GetCheckpointId(connection, checkpointName);

        if (!result)
        {
            AICLI_LOG(Repo, Verbose, << "Did not find checkpoint " << checkpointName);
        }

        return result;
    }

    std::vector<int> CheckpointRecordInterface::GetCheckpointDataTypes(SQLite::Connection& connection, SQLite::rowid_t checkpointId)
    {
        return CheckpointContextTable::GetAvailableData(connection, checkpointId);
    }

    std::vector<std::string> CheckpointRecordInterface::GetCheckpointDataFields(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType)
    {
        return CheckpointContextTable::GetDataFields(connection, checkpointId, dataType);
    }

    bool CheckpointRecordInterface::HasCheckpointDataField(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType, std::string_view name)
    {
        return CheckpointContextTable::HasDataField(connection, checkpointId, dataType, name);
    }
    
    void CheckpointRecordInterface::SetCheckpointDataValues(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType, std::string_view name, std::vector<std::string> values)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addcontextdata_v1_0");

        if (values.empty())
        {
            CheckpointContextTable::AddContextData(connection, checkpointId, dataType, name, {}, 0);
        }
        else
        {
            int index = 0;

            for (const auto& value : values)
            {
                CheckpointContextTable::AddContextData(connection, checkpointId, dataType, name, value, index);
                index++;
            }
        }

        savepoint.Commit();
    }

    std::string CheckpointRecordInterface::GetCheckpointDataValue(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType)
    {
        return CheckpointContextTable::GetDataValue(connection, checkpointId, dataType);
    }

    std::vector<std::string> CheckpointRecordInterface::GetCheckpointDataFieldValues(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int dataType, std::string_view name)
    {
        return CheckpointContextTable::GetDataValuesByFieldName(connection, checkpointId, dataType, name);
    }
}