// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointIndexInterface.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointContextTable.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointMetadataTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    Schema::Version CheckpointIndexInterface::GetVersion() const
    {
        return { 1, 0 };
    }

    void CheckpointIndexInterface::CreateTables(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createCheckpointTables_v1_0");
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

    SQLite::rowid_t CheckpointIndexInterface::SetCommandName(SQLite::Connection& connection, std::string_view commandName)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "setCommandName_v1_0");
        SQLite::rowid_t id = CheckpointMetadataTable::SetCommandName(connection, commandName);
        savepoint.Commit();
        return id;
    }

    std::string CheckpointIndexInterface::GetCommandName(SQLite::Connection& connection)
    {
        return CheckpointMetadataTable::GetCommandName(connection);
    }

    SQLite::rowid_t CheckpointIndexInterface::SetCommandArguments(SQLite::Connection& connection, std::string_view commandArguments)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "setCommandName_v1_0");
        SQLite::rowid_t id = CheckpointMetadataTable::SetCommandArguments(connection, commandArguments);
        savepoint.Commit();
        return id;
    }

    std::string CheckpointIndexInterface::GetCommandArguments(SQLite::Connection& connection)
    {
        return CheckpointMetadataTable::GetCommandArguments(connection);
    }

    bool CheckpointIndexInterface::IsEmpty(SQLite::Connection& connection)
    {
        return CheckpointContextTable::IsEmpty(connection);
    }

    SQLite::rowid_t CheckpointIndexInterface::AddContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData, std::string_view name, std::string_view value)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addContextData_v1_0");
        SQLite::rowid_t rowId = CheckpointContextTable::AddContextData(connection, checkpointName, contextData, name, value);
        savepoint.Commit();
        return rowId;
    }

    std::map<std::string, std::string> CheckpointIndexInterface::GetContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData)
    {
        return CheckpointContextTable::GetContextData(connection, checkpointName, contextData);
    }

    void CheckpointIndexInterface::RemoveContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "removeContextData_v1_0");
        CheckpointContextTable::RemoveContextData(connection, checkpointName, contextData);
        savepoint.Commit();
    }
}