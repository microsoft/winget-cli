// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointIndexInterface.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointTable.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointMetadataTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    namespace
    {
        std::optional<SQLite::rowid_t> GetExistingCommandArgument(const SQLite::Connection& connection, uint32_t type)
        {
            auto result = CheckpointTable::SelectByArgumentType(connection, type);

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
        Checkpoint_V1_0::CheckpointTable::Create(connection);
        Checkpoint_V1_0::CheckpointMetadataTable::Create(connection);
        savepoint.Commit();
    }

    SQLite::rowid_t CheckpointIndexInterface::AddCommandArgument(SQLite::Connection& connection, int type, const std::string_view& argValue)
    {
        auto existingArgument = GetExistingCommandArgument(connection, type);

        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), existingArgument.has_value());

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addCommandArgument_v1_0");
        SQLite::rowid_t argumentId = CheckpointTable::AddCommandArgument(connection, type, argValue);

        savepoint.Commit();
        return argumentId;
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
        return CheckpointTable::IsEmpty(connection);
    }
}