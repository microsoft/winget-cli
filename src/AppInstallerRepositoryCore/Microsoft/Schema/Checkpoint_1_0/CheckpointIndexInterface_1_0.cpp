// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointIndexInterface.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointArgumentsTable.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointContextTable.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointMetadataTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    namespace
    {
        std::optional<SQLite::rowid_t> GetExistingContextRowIdFromArgumentTable(const SQLite::Connection& connection, int contextId)
        {
            auto result = CheckpointArgumentsTable::SelectByContextId(connection, contextId);

            if (!result)
            {
                AICLI_LOG(Repo, Verbose, << "Did not find a context with the contextId  { " << contextId << " }");
            }

            return result;
        }

        std::optional<SQLite::rowid_t> GetExistingContextRowIdFromContextTable(const SQLite::Connection& connection, int contextId)
        {
            auto result = CheckpointContextTable::SelectByContextId(connection, contextId);

            if (!result)
            {
                AICLI_LOG(Repo, Verbose, << "Did not find a context with the contextId  { " << contextId << " }");
            }

            return result;
        }
    }

    Schema::Version CheckpointIndexInterface::GetVersion() const
    {
        return { 1, 0 };
    }

    void CheckpointIndexInterface::CreateTables(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createCheckpointTables_v1_0");
        Checkpoint_V1_0::CheckpointArgumentsTable::Create(connection);
        Checkpoint_V1_0::CheckpointContextTable::Create(connection);
        Checkpoint_V1_0::CheckpointMetadataTable::Create(connection);
        savepoint.Commit();
    }

    SQLite::rowid_t CheckpointIndexInterface::SetClientVersion(SQLite::Connection& connection, std::string_view clientVersion)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "setClientVersion_v1_0");
        SQLite::rowid_t argumentId = CheckpointMetadataTable::SetClientVersion(connection, clientVersion);
        savepoint.Commit();
        return argumentId;
    }
    
    std::string CheckpointIndexInterface::GetClientVersion(SQLite::Connection& connection)
    {
        return CheckpointMetadataTable::GetClientVersion(connection);
    }

    SQLite::rowid_t CheckpointIndexInterface::SetCommandName(SQLite::Connection& connection, int contextId, std::string_view commandName)
    {
        auto contextResult = GetExistingContextRowIdFromArgumentTable(connection, contextId);
        THROW_HR_IF(E_NOT_SET, !contextResult);

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "setCommandName_v1_0");
        SQLite::rowid_t argumentId = CheckpointArgumentsTable::SetCommandNameById(connection, contextResult.value(), commandName);
        savepoint.Commit();
        return argumentId;
    }

    std::string CheckpointIndexInterface::GetCommandName(SQLite::Connection& connection, int contextId)
    {
        auto contextResult = GetExistingContextRowIdFromArgumentTable(connection, contextId);
        THROW_HR_IF(E_NOT_SET, !contextResult);
        return CheckpointArgumentsTable::GetCommandNameById(connection, contextResult.value());
    }

    bool CheckpointIndexInterface::IsEmpty(SQLite::Connection& connection)
    {
        return CheckpointArgumentsTable::IsEmpty(connection) && CheckpointContextTable::IsEmpty(connection);
    }

    SQLite::rowid_t CheckpointIndexInterface::AddContextToArgumentTable(SQLite::Connection& connection, int contextId)
    {
        auto contextResult = GetExistingContextRowIdFromArgumentTable(connection, contextId);
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), contextResult.has_value());

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addContextToArgTable_v1_0");
        SQLite::rowid_t rowId = CheckpointArgumentsTable::AddContext(connection, contextId);
        savepoint.Commit();
        return rowId;
    }

    void CheckpointIndexInterface::RemoveContextFromArgumentTable(SQLite::Connection& connection, int contextId)
    {
        auto contextResult = GetExistingContextRowIdFromArgumentTable(connection, contextId);
        THROW_HR_IF(E_NOT_SET, !contextResult);

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "removeContextFromArgTable_v1_0");
        CheckpointArgumentsTable::RemoveContextById(connection, contextResult.value());
        savepoint.Commit();
    }

    SQLite::rowid_t CheckpointIndexInterface::AddContextToContextTable(SQLite::Connection& connection, int contextId)
    {
        auto contextResult = GetExistingContextRowIdFromContextTable(connection, contextId);
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), contextResult.has_value());

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addContextToContextTable_v1_0");
        SQLite::rowid_t rowId = CheckpointContextTable::AddContext(connection, contextId);
        savepoint.Commit();
        return rowId;
    }

    void CheckpointIndexInterface::RemoveContextFromContextTable(SQLite::Connection& connection, int contextId)
    {
        auto contextResult = GetExistingContextRowIdFromContextTable(connection, contextId);
        THROW_HR_IF(E_NOT_SET, !contextResult);

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "removeContextFromContextTable_v1_0");
        CheckpointContextTable::RemoveContextById(connection, contextResult.value());
        savepoint.Commit();
    }

    bool CheckpointIndexInterface::UpdateArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name, std::string_view value)
    {
        auto contextResult = GetExistingContextRowIdFromArgumentTable(connection, contextId);
        THROW_HR_IF(E_NOT_SET, !contextResult);

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "updateContextArgument_v1_0");
        bool status = CheckpointArgumentsTable::UpdateArgumentById(connection, contextResult.value(), name, value);
        savepoint.Commit();
        return status;
    }

    bool CheckpointIndexInterface::UpdateArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name, bool value)
    {
        auto contextResult = GetExistingContextRowIdFromArgumentTable(connection, contextId);
        THROW_HR_IF(E_NOT_SET, !contextResult);

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "updateContextArgument_v1_0");
        bool status = CheckpointArgumentsTable::UpdateArgumentById(connection, contextResult.value(), name, value);
        savepoint.Commit();
        return status;
    }

    bool CheckpointIndexInterface::ContainsArgument(SQLite::Connection& connection, int contextId, std::string_view name)
    {
        auto contextResult = GetExistingContextRowIdFromArgumentTable(connection, contextId);
        THROW_HR_IF(E_NOT_SET, !contextResult);

        return CheckpointArgumentsTable::ContainsArgument(connection, contextResult.value(), name);
    }

    std::string CheckpointIndexInterface::GetStringArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name)
    {
        auto contextResult = GetExistingContextRowIdFromArgumentTable(connection, contextId);
        THROW_HR_IF(E_NOT_SET, !contextResult);

        return CheckpointArgumentsTable::GetStringArgumentById(connection, contextResult.value(), name);
    }

    bool CheckpointIndexInterface::GetBoolArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name)
    {
        auto contextResult = GetExistingContextRowIdFromArgumentTable(connection, contextId);
        THROW_HR_IF(E_NOT_SET, !contextResult);

        return CheckpointArgumentsTable::GetBoolArgumentById(connection, contextResult.value(), name);
    }

    int CheckpointIndexInterface::GetFirstContextId(SQLite::Connection& connection)
    {
        return CheckpointArgumentsTable::GetFirstContextId(connection);
    }

    bool CheckpointIndexInterface::SetLastCheckpointByContextId(SQLite::Connection& connection, int contextId, int checkpointFlag)
    {
        auto contextResult = GetExistingContextRowIdFromContextTable(connection, contextId);
        THROW_HR_IF(E_NOT_SET, !contextResult);

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "updateLastCheckpointFlag_v1_0");
        bool status = CheckpointContextTable::SetLastCheckpointById(connection, contextResult.value(), checkpointFlag);
        savepoint.Commit();
        return status;
    }

    int CheckpointIndexInterface::GetLastCheckpointByContextId(SQLite::Connection& connection, int contextId)
    {
        auto contextResult = GetExistingContextRowIdFromContextTable(connection, contextId);
        THROW_HR_IF(E_NOT_SET, !contextResult);

        return CheckpointContextTable::GetLastCheckpointById(connection, contextResult.value());
    }
}