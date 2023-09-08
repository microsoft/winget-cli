// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CheckpointContextTable.h"
#include "SQLiteStatementBuilder.h"

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    using namespace SQLite;
    using namespace std::string_view_literals;
    static constexpr std::string_view s_CheckpointContextTable_Table_Name = "CheckpointContext"sv;
    static constexpr std::string_view s_CheckpointContextTable_CheckpointId_Column = "CheckpointId"sv;
    static constexpr std::string_view s_CheckpointContextTable_ContextData_Column = "ContextData"sv;
    static constexpr std::string_view s_CheckpointContextTable_Name_Column = "Name"sv;
    static constexpr std::string_view s_CheckpointContextTable_Value_Column = "Value"sv;
    static constexpr std::string_view s_CheckpointContextTable_Index_Column = "Index"sv;

    namespace
    {
        SQLite::rowid_t SetNamedValue(SQLite::Connection& connection, std::string_view name, std::string_view value)
        {
            SQLite::Builder::StatementBuilder builder;
            builder.InsertInto(s_CheckpointContextTable_Table_Name)
                .Columns({ s_CheckpointContextTable_CheckpointId_Column,
                    s_CheckpointContextTable_ContextData_Column,
                    s_CheckpointContextTable_Name_Column,
                    s_CheckpointContextTable_Value_Column,
                    s_CheckpointContextTable_Index_Column})
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

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createCheckpointContextTable_v1_0");

        StatementBuilder createTableBuilder;
        createTableBuilder.CreateTable(s_CheckpointContextTable_Table_Name).BeginColumns();
        createTableBuilder.Column(ColumnBuilder(s_CheckpointContextTable_CheckpointId_Column, Type::Int).NotNull());
        createTableBuilder.Column(ColumnBuilder(s_CheckpointContextTable_ContextData_Column, Type::Int));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointContextTable_Name_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointContextTable_Value_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointContextTable_Index_Column, Type::Int));
        createTableBuilder.EndColumns();
        createTableBuilder.Execute(connection);
        savepoint.Commit();
    }

    bool CheckpointContextTable::IsEmpty(SQLite::Connection& connection)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::Builder::RowCount).From(s_CheckpointContextTable_Table_Name);

        SQLite::Statement countStatement = builder.Prepare(connection);

        THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

        return (countStatement.GetColumn<int>(0) == 0);
    }

    std::vector<int> CheckpointContextTable::GetAvailableData(SQLite::Connection& connection, SQLite::rowid_t checkpointId)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(s_CheckpointContextTable_ContextData_Column).From(s_CheckpointContextTable_Table_Name).Where(s_CheckpointContextTable_CheckpointId_Column);
        builder.Equals(checkpointId);

        SQLite::Statement select = builder.Prepare(connection);

        std::vector<int> availableData;

        while (select.Step())
        {
            availableData.emplace_back(select.GetColumn<int>(0));
        }

        return availableData;
    }

    SQLite::rowid_t CheckpointContextTable::AddContextData(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int contextData, std::string_view name, std::string_view value, int index)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.InsertInto(s_CheckpointContextTable_Table_Name)
            .Columns({ s_CheckpointContextTable_CheckpointId_Column,
                s_CheckpointContextTable_ContextData_Column,
                s_CheckpointContextTable_Name_Column, 
                s_CheckpointContextTable_Value_Column,
                s_CheckpointContextTable_Index_Column})
            .Values(checkpointId, contextData, name, value, index);

        builder.Execute(connection);
        return connection.GetLastInsertRowID();
    }

    std::vector<std::string> CheckpointContextTable::GetDataFields(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int type)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(s_CheckpointContextTable_Name_Column).From(s_CheckpointContextTable_Table_Name).Where(s_CheckpointContextTable_CheckpointId_Column);
        builder.Equals(checkpointId).And(s_CheckpointContextTable_ContextData_Column).Equals(type);

        SQLite::Statement select = builder.Prepare(connection);

        std::vector<std::string> fields;

        while (select.Step())
        {
            fields.emplace_back(select.GetColumn<std::string>(0));
        }

        return fields;
    }

    std::vector<std::string> CheckpointContextTable::GetDataValuesByName(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int contextData, std::string_view name)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(s_CheckpointContextTable_Value_Column).From(s_CheckpointContextTable_Table_Name).Where(s_CheckpointContextTable_CheckpointId_Column);
        builder.Equals(checkpointId).And(s_CheckpointContextTable_ContextData_Column).Equals(contextData).And(s_CheckpointContextTable_Name_Column).Equals(name);

        SQLite::Statement select = builder.Prepare(connection);

        std::vector<std::string> values;

        while (select.Step())
        {
            values.emplace_back(select.GetColumn<std::string>(0));
        }

        return values;
    }

    bool CheckpointContextTable::HasDataField(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int type, std::string_view name)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::Builder::RowCount).From(s_CheckpointContextTable_Table_Name).Where(s_CheckpointContextTable_CheckpointId_Column);
        builder.Equals(checkpointId).And(s_CheckpointContextTable_ContextData_Column).Equals(type).And(s_CheckpointContextTable_Name_Column).Equals(name);

        SQLite::Statement countStatement = builder.Prepare(connection);

        THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

        return (countStatement.GetColumn<int>(0) == 0);
    }

    void CheckpointContextTable::RemoveContextData(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int contextData)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.DeleteFrom(s_CheckpointContextTable_Table_Name).Where(s_CheckpointContextTable_CheckpointId_Column).Equals(checkpointId)
            .And(s_CheckpointContextTable_ContextData_Column).Equals(contextData);
        builder.Execute(connection);
    }

    std::string CheckpointContextTable::GetSingleDataField(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int type)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(s_CheckpointContextTable_Value_Column).From(s_CheckpointContextTable_Table_Name).Where(s_CheckpointContextTable_CheckpointId_Column);
        builder.Equals(checkpointId).And(s_CheckpointContextTable_ContextData_Column).Equals(type);

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