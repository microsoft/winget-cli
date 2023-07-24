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
    static constexpr std::string_view s_CheckpointTable_ArgumentType_Column = "argumentType"sv;
    static constexpr std::string_view s_CheckpointTable_ArgumentValue_Column = "argumentValue"sv;

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
            ColumnBuilder(s_CheckpointTable_ArgumentType_Column, Type::Int64).NotNull(),
            ColumnBuilder(s_CheckpointTable_ArgumentValue_Column, Type::Text)
            });

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

    std::optional<SQLite::rowid_t> CheckpointTable::SelectByArgumentType(const SQLite::Connection& connection, int type)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::RowIDName).From(s_CheckpointTable_Table_Name).Where(s_CheckpointTable_ArgumentType_Column);
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

    SQLite::rowid_t CheckpointTable::AddCommandArgument(SQLite::Connection& connection, int type, const std::string_view& value)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.InsertInto(s_CheckpointTable_Table_Name)
            .Columns({ s_CheckpointTable_ArgumentType_Column, s_CheckpointTable_ArgumentValue_Column })
            .Values(type, value);

        builder.Execute(connection);
        return connection.GetLastInsertRowID();
    }

    std::vector<std::pair<int, std::string>> CheckpointTable::GetArguments(SQLite::Connection& connection)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select({ s_CheckpointTable_ArgumentType_Column, s_CheckpointTable_ArgumentValue_Column})
            .From(s_CheckpointTable_Table_Name);

        SQLite::Statement select = builder.Prepare(connection);
        std::vector<std::pair<int, std::string>> result;
        while (select.Step())
        {
            auto [argumentType, argumentValue] = select.GetRow<int, std::string>();
            result.emplace_back(std::pair{argumentType, argumentValue});
        }

        return result;
    }
}