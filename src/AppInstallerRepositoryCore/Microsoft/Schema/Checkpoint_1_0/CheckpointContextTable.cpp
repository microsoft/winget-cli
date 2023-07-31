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
    static constexpr std::string_view s_CheckpointContextTable_ContextId_Column = "ContextId"sv;

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
        createTableBuilder.Column(ColumnBuilder(s_CheckpointContextTable_ContextId_Column, Type::Int).PrimaryKey().NotNull());
        createTableBuilder.EndColumns();
        createTableBuilder.Execute(connection);
        savepoint.Commit();
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

    std::optional<SQLite::rowid_t> CheckpointContextTable::SelectByArgumentType(const SQLite::Connection& connection, int type)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::RowIDName).From(s_CheckpointContextTable_Table_Name).Where("id");
        builder.Equals(type);

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

    SQLite::rowid_t CheckpointContextTable::AddContext(SQLite::Connection& connection, int contextId)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.InsertInto(s_CheckpointContextTable_Table_Name)
            .Columns({ s_CheckpointContextTable_ContextId_Column })
            .Values(contextId);

        builder.Execute(connection);
        return connection.GetLastInsertRowID();
    }

    void CheckpointContextTable::RemoveContext(SQLite::Connection& connection, int contextId)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.DeleteFrom(s_CheckpointContextTable_Table_Name).Where(s_CheckpointContextTable_ContextId_Column).Equals(contextId);
        builder.Execute(connection);
    }
}