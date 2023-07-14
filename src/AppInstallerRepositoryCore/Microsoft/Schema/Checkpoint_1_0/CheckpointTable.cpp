// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CheckpointTable.h"
#include "SQLiteStatementBuilder.h"
#include "Microsoft/Schema/ICheckpointIndex.h"

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    using namespace std::string_view_literals;
    static constexpr std::string_view s_CheckpointTable_Table_Name = "Checkpoint"sv;

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

        // Add columns here.

        createTableBuilder.EndColumns();
        createTableBuilder.Execute(connection);

        savepoint.Commit();
    }

    bool CheckpointTable::ExistsById(const SQLite::Connection& connection, SQLite::rowid_t id)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::Builder::RowCount).From(s_CheckpointTable_Table_Name).Where(SQLite::RowIDName).Equals(id);

        SQLite::Statement countStatement = builder.Prepare(connection);

        THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

        return (countStatement.GetColumn<int>(0) != 0);
    }

    void CheckpointTable::DeleteById(SQLite::Connection& connection, SQLite::rowid_t id)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.DeleteFrom(s_CheckpointTable_Table_Name).Where(SQLite::RowIDName).Equals(id);

        builder.Execute(connection);
    }

    bool CheckpointTable::IsEmpty(SQLite::Connection& connection)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::Builder::RowCount).From(s_CheckpointTable_Table_Name);

        SQLite::Statement countStatement = builder.Prepare(connection);

        THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

        return (countStatement.GetColumn<int>(0) == 0);
    }
}