// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CheckpointContextTable.h"
#include "SQLiteStatementBuilder.h"
#include "Microsoft/Schema/ICheckpointIndex.h"

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    using namespace SQLite;
    using namespace std::string_view_literals;
    static constexpr std::string_view s_CheckpointContextTable_Table_Name = "CheckpointContext"sv;
    static constexpr std::string_view s_CheckpointContextTable_CheckpointName_Column = "CheckpointName"sv;
    static constexpr std::string_view s_CheckpointContextTable_ContextData_Column = "ContextData"sv;
    static constexpr std::string_view s_CheckpointContextTable_Name_Column = "Name"sv;
    static constexpr std::string_view s_CheckpointContextTable_Value_Column = "Value"sv;

    static constexpr std::string_view s_CheckpointMetadataTable_CheckpointName = "CheckpointName"sv;
    static constexpr std::string_view s_CheckpointMetadataTable_ClientVersion = "ClientVersion"sv;
    static constexpr std::string_view s_CheckpointMetadataTable_CommandName = "CommandName"sv;
    static constexpr std::string_view s_CheckpointMetadataTable_CommandArguments = "CommandArguments"sv;
    static constexpr std::string_view s_CheckpointMetadataTable_CommitTime = "CommandArguments"sv;

    // Everytime we commit to the table we write here:


    std::string_view CheckpointContextTable::TableName()
    {
        return s_CheckpointContextTable_Table_Name;
    }

    void CheckpointContextTable::Create(SQLite::Connection& connection)
    {
        using namespace SQLite::Builder;

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createCheckpointArgumentsTable_v1_0");

        StatementBuilder createTableBuilder;
        createTableBuilder.CreateTable(s_CheckpointContextTable_Table_Name).BeginColumns();
        createTableBuilder.Column(ColumnBuilder(s_CheckpointContextTable_CheckpointName_Column, Type::Text).NotNull());
        createTableBuilder.Column(ColumnBuilder(s_CheckpointContextTable_ContextData_Column, Type::Int));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointContextTable_Name_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointContextTable_Value_Column, Type::Text));
        createTableBuilder.EndColumns();
        createTableBuilder.Execute(connection);
        savepoint.Commit();
    }

    std::optional<SQLite::rowid_t> CheckpointContextTable::SelectByCheckpointName(const SQLite::Connection& connection, std::string checkpointName)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::RowIDName).From(s_CheckpointContextTable_Table_Name).Where(s_CheckpointContextTable_ContextId_Column);
        builder.Equals(contextId);

        SQLite::Statement select = builder.Prepare(connection);

        if (select.Step())
        {
            return select.GetColumn<SQLite::rowid_t>(0);
        }
        else
        {
            return {};
        }
    }

    bool CheckpointContextTable::ExistsById(const SQLite::Connection& connection, SQLite::rowid_t id)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::Builder::RowCount).From(s_CheckpointContextTable_Table_Name).Where(SQLite::RowIDName).Equals(id);

        SQLite::Statement countStatement = builder.Prepare(connection);

        THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

        return (countStatement.GetColumn<int>(0) != 0);
    }

    void CheckpointContextTable::DeleteById(SQLite::Connection& connection, SQLite::rowid_t id)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.DeleteFrom(s_CheckpointContextTable_Table_Name).Where(SQLite::RowIDName).Equals(id);

        builder.Execute(connection);
    }

    bool CheckpointContextTable::IsEmpty(SQLite::Connection& connection)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::Builder::RowCount).From(s_CheckpointContextTable_Table_Name);

        SQLite::Statement countStatement = builder.Prepare(connection);

        THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

        return (countStatement.GetColumn<int>(0) == 0);
    }

    SQLite::rowid_t CheckpointContextTable::AddContext(SQLite::Connection& connection, int contextId)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.InsertInto(s_CheckpointContextTable_Table_Name)
            .Columns({ s_CheckpointContextTable_ContextId_Column })
            .Values(contextId);

        builder.Execute(connection);
        return connection.GetLastInsertRowID();
    }

    void CheckpointContextTable::RemoveContextById(SQLite::Connection& connection, SQLite::rowid_t id)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.DeleteFrom(s_CheckpointContextTable_Table_Name).Where(SQLite::RowIDName).Equals(id);
        builder.Execute(connection);
    }

    bool CheckpointContextTable::SetLastCheckpointById(SQLite::Connection& connection, SQLite::rowid_t id, int checkpointFlag)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Update(s_CheckpointContextTable_Table_Name).Set()
            .Column(s_CheckpointContextTable_LastCheckpoint_Column).Equals(checkpointFlag)
            .Where(SQLite::RowIDName).Equals(id);

        builder.Execute(connection);
        return connection.GetChanges() != 0;
    }

    int CheckpointContextTable::GetLastCheckpointById(SQLite::Connection& connection, SQLite::rowid_t id)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(s_CheckpointContextTable_LastCheckpoint_Column).From(s_CheckpointContextTable_Table_Name).
            Where(SQLite::RowIDName).Equals(id);

        Statement statement = builder.Prepare(connection);
        if (statement.Step())
        {
            return statement.GetColumn<int>(0);
        }
        else
        {
            return {};
        }
    }
}