// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CheckpointMetadataTable.h"
#include "SQLiteStatementBuilder.h"
#include "Microsoft/Schema/ICheckpointIndex.h"

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    using namespace std::string_view_literals;
    static constexpr std::string_view s_CheckpointMetadataTable_Table_Name = "CheckpointMetadata"sv;
    static constexpr std::string_view s_CheckpointMetadataTable_Name_Column = "Name";
    static constexpr std::string_view s_CheckpointMetadataTable_Value_Column = "Value";

    static constexpr std::string_view s_CheckpointMetadataTable_ClientVersion = "ClientVersion"sv;
    static constexpr std::string_view s_CheckpointMetadataTable_CommandName = "CommandName"sv;
    static constexpr std::string_view s_CheckpointMetadataTable_CommandArguments = "CommandArguments"sv;

    namespace
    {
        SQLite::rowid_t SetNamedValue(SQLite::Connection& connection, std::string_view name, std::string_view value)
        {
            SQLite::Builder::StatementBuilder builder;
            builder.InsertInto(s_CheckpointMetadataTable_Table_Name)
                .Columns({ s_CheckpointMetadataTable_Name_Column,
                    s_CheckpointMetadataTable_Value_Column })
                .Values(name, value);

            builder.Execute(connection);
            return connection.GetLastInsertRowID();
        }

        std::string GetNamedValue(SQLite::Connection& connection, std::string_view name)
        {
            SQLite::Builder::StatementBuilder builder;
            builder.Select({ s_CheckpointMetadataTable_Value_Column })
                .From(s_CheckpointMetadataTable_Table_Name).Where(s_CheckpointMetadataTable_Name_Column).Equals(name);

            SQLite::Statement statement = builder.Prepare(connection);
            THROW_HR_IF(E_NOT_SET, !statement.Step());
            return statement.GetColumn<std::string>(0);
        }
    }

    std::string_view CheckpointMetadataTable::TableName()
    {
        return s_CheckpointMetadataTable_Table_Name;
    }
    
    void CheckpointMetadataTable::Create(SQLite::Connection& connection)
    {
        using namespace SQLite::Builder;

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createCheckpointMetadataTable_v1_0");

        StatementBuilder createTableBuilder;
        createTableBuilder.CreateTable(s_CheckpointMetadataTable_Table_Name).BeginColumns();
        createTableBuilder.Column(ColumnBuilder(s_CheckpointMetadataTable_Name_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointMetadataTable_Value_Column, Type::Text));
        createTableBuilder.EndColumns();
        createTableBuilder.Execute(connection);

        savepoint.Commit();
    }

    SQLite::rowid_t CheckpointMetadataTable::SetClientVersion(SQLite::Connection& connection, std::string_view clientVersion)
    {
        return SetNamedValue(connection, s_CheckpointMetadataTable_ClientVersion, clientVersion);
    }

    std::string CheckpointMetadataTable::GetClientVersion(SQLite::Connection& connection)
    {
        return GetNamedValue(connection, s_CheckpointMetadataTable_ClientVersion);
    }

    SQLite::rowid_t CheckpointMetadataTable::SetCommandName(SQLite::Connection& connection, std::string_view commandName)
    {
        return SetNamedValue(connection, s_CheckpointMetadataTable_CommandName, commandName);
    }

    std::string CheckpointMetadataTable::GetCommandName(SQLite::Connection& connection)
    {
        return GetNamedValue(connection, s_CheckpointMetadataTable_CommandName);
    }

    SQLite::rowid_t CheckpointMetadataTable::SetCommandArguments(SQLite::Connection& connection, std::string_view commandArguments)
    {
        return SetNamedValue(connection, s_CheckpointMetadataTable_CommandArguments, commandArguments);
    }

    std::string CheckpointMetadataTable::GetCommandArguments(SQLite::Connection& connection)
    {
        return GetNamedValue(connection, s_CheckpointMetadataTable_CommandArguments);
    }
}