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

    namespace
    {
        SQLite::rowid_t SetNamedValue(SQLite::Connection& connection, std::string_view name, std::string_view value)
        {
            SQLite::Builder::StatementBuilder builder;
            builder.InsertInto(s_CheckpointContextTable_Table_Name)
                .Columns({ s_CheckpointContextTable_CheckpointName_Column,
                    s_CheckpointContextTable_ContextData_Column,
                    s_CheckpointContextTable_Name_Column,
                    s_CheckpointContextTable_Value_Column })
                .Values(name, value);

            builder.Execute(connection);
            return connection.GetLastInsertRowID();
        }

        std::string GetNamedValue(SQLite::Connection& connection, std::string_view name)
        {
            SQLite::Builder::StatementBuilder builder;
            builder.Select({ s_CheckpointContextTable_Value_Column })
                .From(s_CheckpointContextTable_Table_Name).Where(s_CheckpointContextTable_Name_Column).Equals(name);

            SQLite::Statement statement = builder.Prepare(connection);
            THROW_HR_IF(E_NOT_SET, !statement.Step());
            return statement.GetColumn<std::string>(0);
        }
    }

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

    SQLite::rowid_t CheckpointContextTable::AddCheckpoint(SQLite::Connection& connection, std::string_view checkpointName)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.InsertInto(s_CheckpointContextTable_Table_Name)
            .Columns({ s_CheckpointContextTable_CheckpointName_Column })
            .Values(checkpointName);

        builder.Execute(connection);
        return connection.GetLastInsertRowID();
    }

    SQLite::rowid_t CheckpointContextTable::AddContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData, std::string_view name, std::string_view value)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.InsertInto(s_CheckpointContextTable_Table_Name)
            .Columns({ s_CheckpointContextTable_CheckpointName_Column,
                s_CheckpointContextTable_ContextData_Column,
                s_CheckpointContextTable_Name_Column, 
                s_CheckpointContextTable_Value_Column})
            .Values(checkpointName, contextData, name, value);

        builder.Execute(connection);
        return connection.GetLastInsertRowID();
    }

    void CheckpointContextTable::RemoveContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::RowIDName).From(s_CheckpointContextTable_Table_Name).Where(s_CheckpointContextTable_CheckpointName_Column).Equals(checkpointName)
            .And(s_CheckpointContextTable_ContextData_Column).Equals(contextData);

        SQLite::Statement select = builder.Prepare(connection);
        while (select.Step())
        {
            DeleteById(connection, select.GetColumn<SQLite::rowid_t>(0));
        }
    }

    std::string CheckpointContextTable::GetContextData(SQLite::Connection& connection, std::string_view checkpointName, int contextData, std::string_view name)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(s_CheckpointContextTable_Value_Column).From(s_CheckpointContextTable_Table_Name).Where(s_CheckpointContextTable_CheckpointName_Column);
        builder.Equals(checkpointName).And(s_CheckpointContextTable_ContextData_Column).Equals(contextData).And(s_CheckpointContextTable_Name_Column).Equals(name);

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
}