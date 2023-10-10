// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MetadataTable.h"


namespace AppInstaller::Repository::Microsoft::Schema
{
    // Table data [note that this table is not versioned, and thus *cannot change*]
    static constexpr std::string_view s_MetadataTable_Table_Name = "metadata"sv;
    static constexpr std::string_view s_MetadataTable_Column_Name = "name"sv;
    static constexpr std::string_view s_MetadataTable_Column_Value = "value"sv;

    static constexpr std::string_view s_MetadataTable_Table_Create = R"(
CREATE TABLE [metadata](
    [name] TEXT PRIMARY KEY NOT NULL,
    [value] TEXT NOT NULL)
)"sv;

    // Statements
    static constexpr std::string_view s_MetadataTableStmt_GetNamedValue = "select [value] from [metadata] where [name] = ?"sv;
    static constexpr std::string_view s_MetadataTableStmt_SetNamedValue = "insert or replace into [metadata] ([name], [value]) values (?, ?)"sv;

    void MetadataTable::Create(SQLite::Connection& connection)
    {
        SQLite::Statement create = SQLite::Statement::Create(connection, s_MetadataTable_Table_Create);
        create.Execute();
    }

    SQLite::Statement MetadataTable::GetNamedValueStatement(const SQLite::Connection& connection, std::string_view name)
    {
        std::optional<SQLite::Statement> result = TryGetNamedValueStatement(connection, name);
        THROW_HR_IF(E_NOT_SET, !result);
        return std::move(result).value();
    }

    std::optional<SQLite::Statement> MetadataTable::TryGetNamedValueStatement(const SQLite::Connection& connection, std::string_view name)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());

        SQLite::Statement result = SQLite::Statement::Create(connection, s_MetadataTableStmt_GetNamedValue);
        result.Bind(1, name);

        if (result.Step())
        {
            return result;
        }
        else
        {
            return std::nullopt;
        }
    }

    SQLite::Statement MetadataTable::SetNamedValueStatement(const SQLite::Connection& connection, std::string_view name)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());
        SQLite::Statement result = SQLite::Statement::Create(connection, s_MetadataTableStmt_SetNamedValue);
        result.Bind(1, name);
        return result;
    }
}
