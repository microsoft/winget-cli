// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CheckpointDataTable.h"
#include <winget/SQLiteStatementBuilder.h>

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    using namespace SQLite;
    using namespace std::string_view_literals;
    static constexpr std::string_view s_CheckpointDataTable_Table_Name = "CheckpointData"sv;
    static constexpr std::string_view s_CheckpointDataTable_CheckpointId_Column = "CheckpointId"sv;
    static constexpr std::string_view s_CheckpointDataTable_ContextData_Column = "ContextData"sv;
    static constexpr std::string_view s_CheckpointDataTable_Name_Column = "Name"sv;
    static constexpr std::string_view s_CheckpointDataTable_Value_Column = "Value"sv;
    static constexpr std::string_view s_CheckpointDataTable_Index_Column = "Index"sv;

    namespace
    {
        SQLite::rowid_t SetNamedValue(SQLite::Connection& connection, std::string_view name, std::string_view value)
        {
            SQLite::Builder::StatementBuilder builder;
            builder.InsertInto(s_CheckpointDataTable_Table_Name)
                .Columns({ s_CheckpointDataTable_CheckpointId_Column,
                    s_CheckpointDataTable_ContextData_Column,
                    s_CheckpointDataTable_Name_Column,
                    s_CheckpointDataTable_Value_Column,
                    s_CheckpointDataTable_Index_Column})
                .Values(name, value);

            builder.Execute(connection);
            return connection.GetLastInsertRowID();
        }

        std::string GetNamedValue(SQLite::Connection& connection, std::string_view name)
        {
            SQLite::Builder::StatementBuilder builder;
            builder.Select({ s_CheckpointDataTable_Value_Column })
                .From(s_CheckpointDataTable_Table_Name).Where(s_CheckpointDataTable_Name_Column).Equals(name);

            SQLite::Statement statement = builder.Prepare(connection);
            THROW_HR_IF(E_NOT_SET, !statement.Step());
            return statement.GetColumn<std::string>(0);
        }
    }

    std::string_view CheckpointDataTable::TableName()
    {
        return s_CheckpointDataTable_Table_Name;
    }

    void CheckpointDataTable::Create(SQLite::Connection& connection)
    {
        using namespace SQLite::Builder;

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createCheckpointDataTable_v1_0");

        StatementBuilder createTableBuilder;
        createTableBuilder.CreateTable(s_CheckpointDataTable_Table_Name).BeginColumns();
        createTableBuilder.Column(ColumnBuilder(s_CheckpointDataTable_CheckpointId_Column, Type::Int).NotNull());
        createTableBuilder.Column(ColumnBuilder(s_CheckpointDataTable_ContextData_Column, Type::Int).NotNull());
        createTableBuilder.Column(ColumnBuilder(s_CheckpointDataTable_Name_Column, Type::Text).NotNull());
        createTableBuilder.Column(ColumnBuilder(s_CheckpointDataTable_Value_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointDataTable_Index_Column, Type::Int).NotNull());

        PrimaryKeyBuilder pkBuilder;
        pkBuilder.Column(s_CheckpointDataTable_CheckpointId_Column);
        pkBuilder.Column(s_CheckpointDataTable_ContextData_Column);
        pkBuilder.Column(s_CheckpointDataTable_Name_Column);
        pkBuilder.Column(s_CheckpointDataTable_Index_Column);

        createTableBuilder.Column(pkBuilder).EndColumns();
        createTableBuilder.Execute(connection);
        savepoint.Commit();
    }

    bool CheckpointDataTable::IsEmpty(SQLite::Connection& connection)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::Builder::RowCount).From(s_CheckpointDataTable_Table_Name);

        SQLite::Statement countStatement = builder.Prepare(connection);

        THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

        return (countStatement.GetColumn<int>(0) == 0);
    }

    std::vector<int> CheckpointDataTable::GetAvailableData(SQLite::Connection& connection, SQLite::rowid_t checkpointId)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(s_CheckpointDataTable_ContextData_Column).From(s_CheckpointDataTable_Table_Name).Where(s_CheckpointDataTable_CheckpointId_Column);
        builder.Equals(checkpointId);

        SQLite::Statement select = builder.Prepare(connection);

        std::vector<int> availableData;

        while (select.Step())
        {
            availableData.emplace_back(select.GetColumn<int>(0));
        }

        return availableData;
    }

    SQLite::rowid_t CheckpointDataTable::AddCheckpointData(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int contextData, std::string_view name, std::string_view value, int index)
    {
        SQLite::Builder::StatementBuilder builder;

        if (value.empty())
        {
            builder.InsertInto(s_CheckpointDataTable_Table_Name)
                .Columns({ s_CheckpointDataTable_CheckpointId_Column,
                    s_CheckpointDataTable_ContextData_Column,
                    s_CheckpointDataTable_Name_Column,
                    s_CheckpointDataTable_Index_Column })
                .Values(checkpointId, contextData, name, index);
        }
        else
        {
            builder.InsertInto(s_CheckpointDataTable_Table_Name)
                .Columns({ s_CheckpointDataTable_CheckpointId_Column,
                    s_CheckpointDataTable_ContextData_Column,
                    s_CheckpointDataTable_Name_Column,
                    s_CheckpointDataTable_Value_Column,
                    s_CheckpointDataTable_Index_Column })
                .Values(checkpointId, contextData, name, value, index);
        }

        builder.Execute(connection);
        return connection.GetLastInsertRowID();
    }

    bool CheckpointDataTable::HasDataField(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int type, std::string_view name)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::Builder::RowCount).From(s_CheckpointDataTable_Table_Name).Where(s_CheckpointDataTable_CheckpointId_Column);
        builder.Equals(checkpointId).And(s_CheckpointDataTable_ContextData_Column).Equals(type).And(s_CheckpointDataTable_Name_Column).Equals(name);

        SQLite::Statement countStatement = builder.Prepare(connection);

        THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

        return (countStatement.GetColumn<int>(0) == 0);
    }

    std::vector<std::string> CheckpointDataTable::GetDataFields(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int type)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(s_CheckpointDataTable_Name_Column).From(s_CheckpointDataTable_Table_Name).Where(s_CheckpointDataTable_CheckpointId_Column);
        builder.Equals(checkpointId).And(s_CheckpointDataTable_ContextData_Column).Equals(type);

        SQLite::Statement select = builder.Prepare(connection);

        std::vector<std::string> fields;

        while (select.Step())
        {
            fields.emplace_back(select.GetColumn<std::string>(0));
        }

        return fields;
    }

    std::vector<std::string> CheckpointDataTable::GetDataValuesByFieldName(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int contextData, std::string_view name)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(s_CheckpointDataTable_Value_Column).From(s_CheckpointDataTable_Table_Name).Where(s_CheckpointDataTable_CheckpointId_Column);
        builder.Equals(checkpointId).And(s_CheckpointDataTable_ContextData_Column).Equals(contextData).And(s_CheckpointDataTable_Name_Column).Equals(name);

        SQLite::Statement select = builder.Prepare(connection);

        std::vector<std::string> values;

        while (select.Step())
        {
            values.emplace_back(select.GetColumn<std::string>(0));
        }

        return values;
    }

    std::string CheckpointDataTable::GetDataValue(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int type)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(s_CheckpointDataTable_Value_Column).From(s_CheckpointDataTable_Table_Name).Where(s_CheckpointDataTable_CheckpointId_Column);
        builder.Equals(checkpointId).And(s_CheckpointDataTable_ContextData_Column).Equals(type);

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

    void CheckpointDataTable::RemoveDataType(SQLite::Connection& connection, SQLite::rowid_t checkpointId, int contextData)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.DeleteFrom(s_CheckpointDataTable_Table_Name).Where(s_CheckpointDataTable_CheckpointId_Column).Equals(checkpointId)
            .And(s_CheckpointDataTable_ContextData_Column).Equals(contextData);
        builder.Execute(connection);
    }
}