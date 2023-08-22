// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CheckpointTable.h"
#include "SQLiteStatementBuilder.h"

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    using namespace std::string_view_literals;
    static constexpr std::string_view s_CheckpointTable_Table_Name = "Checkpoints"sv;
    static constexpr std::string_view s_CheckpointTable_Name_Column = "Name";
    static constexpr std::string_view s_CheckpointTable_WriteTime_Column = "WriteTime";

    std::string_view CheckpointTable::TableName()
    {
        return s_CheckpointTable_Table_Name;
    }
    
    void CheckpointTable::Create(SQLite::Connection& connection)
    {
        using namespace SQLite::Builder;

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createCheckpointTable_v1_0");

        StatementBuilder createTableBuilder;
        createTableBuilder.CreateTable(s_CheckpointTable_Table_Name).BeginColumns();
        createTableBuilder.Column(ColumnBuilder(s_CheckpointTable_Name_Column, Type::Text).Unique());
        createTableBuilder.Column(ColumnBuilder(s_CheckpointTable_WriteTime_Column, Type::Int64));
        createTableBuilder.EndColumns();
        createTableBuilder.Execute(connection);

        savepoint.Commit();
    }

    std::string CheckpointTable::GetLastCheckpoint(SQLite::Connection& connection)
    {
        // Sort by descending and get last checkpoint.
        SQLite::Builder::StatementBuilder builder;
        builder.Select(s_CheckpointTable_Name_Column).From(s_CheckpointTable_Table_Name).OrderBy(SQLite::RowIDName).Descending();

        SQLite::Statement select = builder.Prepare(connection);

        if (select.Step())
        {
            return select.GetColumn<std::string>(0);
        }
        else
        {
            return {};
        }
    }

    SQLite::rowid_t CheckpointTable::AddCheckpoint(SQLite::Connection& connection, std::string_view checkpointName)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.InsertInto(s_CheckpointTable_Table_Name)
            .Columns({ s_CheckpointTable_Name_Column,
                s_CheckpointTable_WriteTime_Column })
            .Values(checkpointName, Utility::GetCurrentUnixEpoch());

        builder.Execute(connection);
        return connection.GetLastInsertRowID();
    }

    std::optional<SQLite::rowid_t> CheckpointTable::GetCheckpointId(SQLite::Connection& connection, std::string_view checkpointName)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::RowIDName).From(s_CheckpointTable_Table_Name).Where(s_CheckpointTable_Name_Column).Equals(checkpointName);

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
}