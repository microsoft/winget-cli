// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CheckpointTable.h"
#include <winget/SQLiteStatementBuilder.h>

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    using namespace std::string_view_literals;
    static constexpr std::string_view s_CheckpointTable_Table_Name = "Checkpoints"sv;
    static constexpr std::string_view s_CheckpointTable_Index_Name = "Checkpoints_pkindex"sv;
    static constexpr std::string_view s_CheckpointTable_Name_Column = "Name";
    static constexpr std::string_view s_CheckpointTable_CreationTime_Column = "CreationTime";

    std::string_view CheckpointTable::TableName()
    {
        return s_CheckpointTable_Table_Name;
    }
    
    void CheckpointTable::Create(SQLite::Connection& connection)
    {
        using namespace SQLite::Builder;

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createCheckpointTable_v1_0");

        StatementBuilder createTableBuilder;
        createTableBuilder.CreateTable(s_CheckpointTable_Table_Name).Columns({
            ColumnBuilder(s_CheckpointTable_Index_Name, Type::Integer).PrimaryKey(),
            ColumnBuilder(s_CheckpointTable_Name_Column, Type::Text).NotNull(),
            ColumnBuilder(s_CheckpointTable_CreationTime_Column, Type::Int64).NotNull()
            });

        createTableBuilder.Execute(connection);

        savepoint.Commit();
    }

    std::vector<SQLite::rowid_t> CheckpointTable::GetCheckpointIds(SQLite::Connection& connection)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::RowIDName).From(s_CheckpointTable_Table_Name).OrderBy(SQLite::RowIDName).Descending();

        SQLite::Statement select = builder.Prepare(connection);

        std::vector<SQLite::rowid_t> checkpoints;

        while (select.Step())
        {
            checkpoints.emplace_back(select.GetColumn<SQLite::rowid_t>(0));
        }

        return checkpoints;
    }

    SQLite::rowid_t CheckpointTable::AddCheckpoint(SQLite::Connection& connection, std::string_view checkpointName)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.InsertInto(s_CheckpointTable_Table_Name)
            .Columns({ s_CheckpointTable_Name_Column,
                s_CheckpointTable_CreationTime_Column })
            .Values(checkpointName, Utility::GetCurrentUnixEpoch());

        builder.Execute(connection);
        return connection.GetLastInsertRowID();
    }
}