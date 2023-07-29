// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointIndexInterface.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointArgumentsTable.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointMetadataTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    namespace
    {
        std::optional<SQLite::rowid_t> GetExistingCommandArgument(const SQLite::Connection& connection, uint32_t type)
        {
            auto result = CheckpointArgumentsTable::SelectByArgumentType(connection, type);

            if (!result)
            {
                AICLI_LOG(Repo, Verbose, << "Did not find an argument with the type { " << type << " }");
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
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createCheckpointTable_v1_0");
        Checkpoint_V1_0::CheckpointArgumentsTable::Create(connection);
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

    SQLite::rowid_t CheckpointIndexInterface::SetCommandName(SQLite::Connection& connection, std::string_view commandName)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "setCommandName_v1_0");
        SQLite::rowid_t argumentId = CheckpointMetadataTable::SetCommandName(connection, commandName);

        savepoint.Commit();
        return argumentId;
    }

    std::string CheckpointIndexInterface::GetCommandName(SQLite::Connection& connection)
    {
        return CheckpointMetadataTable::GetCommandName(connection);
    }

    bool CheckpointIndexInterface::IsEmpty(SQLite::Connection& connection)
    {
        return CheckpointArgumentsTable::IsEmpty(connection);
    }

    SQLite::rowid_t CheckpointIndexInterface::AddContextToArgumentTable(SQLite::Connection& connection, int contextId)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addContextToArgTable_v1_0");
        SQLite::rowid_t rowId = CheckpointArgumentsTable::AddContext(connection, contextId);
        savepoint.Commit();
        return rowId;
    }

    void CheckpointIndexInterface::RemoveContextFromArgumentTable(SQLite::Connection& connection, int contextId)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "removeContextFromArgTable_v1_0");
        CheckpointArgumentsTable::RemoveContext(connection, contextId);
        savepoint.Commit();
    }

    bool CheckpointIndexInterface::UpdateArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name, std::string_view value)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "updateContextArgument_v1_0");
        bool status = CheckpointArgumentsTable::UpdateArgumentByContextId(connection, contextId, name, value);
        savepoint.Commit();
        return status;
    }

    bool CheckpointIndexInterface::UpdateArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name, bool value)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "updateContextArgument_v1_0");
        bool status = CheckpointArgumentsTable::UpdateArgumentByContextId(connection, contextId, name, value);
        savepoint.Commit();
        return status;
    }

    std::vector<std::string> CheckpointIndexInterface::GetAvailableArguments(SQLite::Connection& connection, int contextId)
    {
        return CheckpointArgumentsTable::GetAvailableArguments(connection, contextId);
    }

    bool CheckpointIndexInterface::ContainsArgument(SQLite::Connection& connection, int contextId, std::string_view name)
    {
        return CheckpointArgumentsTable::ContainsArgument(connection, contextId, name);
    }

    std::string CheckpointIndexInterface::GetStringArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name)
    {
        return CheckpointArgumentsTable::GetStringArgumentByContextId(connection, contextId, name);
    }

    bool CheckpointIndexInterface::GetBoolArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name)
    {
        return CheckpointArgumentsTable::GetBoolArgumentByContextId(connection, contextId, name);
    }
}