// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointRecordInterface.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointContextTable.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointMetadataTable.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    namespace
    {
        std::optional<SQLite::rowid_t> GetExistingCheckpointId(SQLite::Connection& connection, std::string_view checkpointName)
        {
            auto result = CheckpointTable::GetCheckpointId(connection, checkpointName);

            if (!result)
            {
                AICLI_LOG(Repo, Verbose, << "Did not find checkpoint " << checkpointName);
            }

            return result;
        }
    }

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

    bool CheckpointRecordInterface::IsEmpty(SQLite::Connection& connection)
    {
        return CheckpointContextTable::IsEmpty(connection);
    }


    


    SQLite::rowid_t CheckpointRecordInterface::SetMetadata(SQLite::Connection& connection, std::string_view name, std::string_view value)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "setMetadata_v1_0");
        SQLite::rowid_t argumentId = CheckpointMetadataTable::SetNamedValue(connection, name, value);
        savepoint.Commit();
        return argumentId;
    }

    bool CheckpointRecordInterface::CheckpointExists(SQLite::Connection& connection, std::string_view checkpointName)
    {
        return GetExistingCheckpointId(connection, checkpointName).has_value();
    }

    std::string CheckpointRecordInterface::GetMetadata(SQLite::Connection& connection, std::string_view name)
    {
        return CheckpointMetadataTable::GetNamedValue(connection, name);
    }

    std::string CheckpointRecordInterface::GetLastCheckpoint(SQLite::Connection& connection)
    {
        return CheckpointTable::GetLastCheckpoint(connection);
    }

    std::vector<int> CheckpointRecordInterface::GetAvailableContextData(SQLite::Connection& connection, std::string_view checkpointName)
    {
        auto existingCheckpointId = GetExistingCheckpointId(connection, checkpointName);
        if (!existingCheckpointId)
        {
            return {};
        }

        return CheckpointContextTable::GetAvailableData(connection, existingCheckpointId.value());
    }

    SQLite::rowid_t CheckpointRecordInterface::AddCheckpoint(SQLite::Connection& connection, std::string_view checkpointName)
    {
        auto existingCheckpointId = GetExistingCheckpointId(connection, checkpointName);
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), existingCheckpointId);

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addcheckpoint_v1_0");
        SQLite::rowid_t checkpointId = CheckpointTable::AddCheckpoint(connection, checkpointName);
        savepoint.Commit();
        return checkpointId;
    }

    SQLite::rowid_t CheckpointRecordInterface::AddContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData, std::string_view name, std::string_view value, int index)
    {
        auto existingCheckpointId = GetExistingCheckpointId(connection, checkpointName);
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_NOT_FOUND), !existingCheckpointId);

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addcontextdata_v1_0");
        SQLite::rowid_t rowId = CheckpointContextTable::AddContextData(connection, existingCheckpointId.value(), contextData, name, value, index);
        savepoint.Commit();
        return rowId;
    }

    std::vector<std::string> CheckpointRecordInterface::GetContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData)
    {
        auto existingCheckpointId = GetExistingCheckpointId(connection, checkpointName);
        if (!existingCheckpointId)
        {
            return {};
        }

        return CheckpointContextTable::GetContextData(connection, existingCheckpointId.value(), contextData);
    }

    std::vector<std::string> CheckpointRecordInterface::GetContextDataByName(SQLite::Connection& connection, std::string_view checkpointName, int contextData, std::string_view name)
    {
        auto existingCheckpointId = GetExistingCheckpointId(connection, checkpointName);
        if (!existingCheckpointId)
        {
            return {};
        }

        return CheckpointContextTable::GetContextDataByName(connection, existingCheckpointId.value(), contextData, name);
    }

    void CheckpointRecordInterface::RemoveContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData)
    {
        auto existingCheckpointId = GetExistingCheckpointId(connection, checkpointName);
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_NOT_FOUND), !existingCheckpointId);

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "removecontextdata_v1_0");
        CheckpointContextTable::RemoveContextData(connection, existingCheckpointId.value(), contextData);
        savepoint.Commit();
    }
}