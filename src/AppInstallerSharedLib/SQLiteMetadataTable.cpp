// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/SQLiteMetadataTable.h"

using namespace std::literals;

namespace AppInstaller::SQLite
{
    // Table data [note that this table is not versioned, and thus *cannot change*]
    static constexpr std::string_view s_MetadataTable_Table_Name = "metadata"sv;
    static constexpr std::string_view s_MetadataTable_Column_Name = "name"sv;
    static constexpr std::string_view s_MetadataTable_Column_Value = "value"sv;

    static constexpr std::string_view s_MetadataTable_Table_Create = R"(
CREATE TABLE [metadata](
    [name] TEXT PRIMARY KEY NOT NULL,
    [value] TEXT NOT NULL) WITHOUT ROWID
)"sv;

    // Statements
    static constexpr std::string_view s_MetadataTableStmt_GetNamedValue = "select [value] from [metadata] where [name] = ?"sv;
    static constexpr std::string_view s_MetadataTableStmt_SetNamedValue = "insert or replace into [metadata] ([name], [value]) values (?, ?)"sv;

    namespace
    {
        // The statement to read a named value from the given database, which is the alias an
        // attached database was given. The unqualified form is used for the primary database so
        // that the overwhelmingly common path stays the fixed string it has always been.
        std::string GetNamedValueSql(std::string_view database)
        {
            if (database.empty())
            {
                return std::string{ s_MetadataTableStmt_GetNamedValue };
            }

            std::ostringstream result;
            result << "select [value] from [" << database << "].[metadata] where [name] = ?";
            return std::move(result).str();
        }
    }

    void MetadataTable::Create(Connection& connection)
    {
        Statement create = Statement::Create(connection, s_MetadataTable_Table_Create);
        create.Execute();
    }

    Statement MetadataTable::GetNamedValueStatement(const Connection& connection, std::string_view name, std::string_view database)
    {
        std::optional<Statement> result = TryGetNamedValueStatement(connection, name, database);
        THROW_HR_IF(E_NOT_SET, !result);
        return std::move(result).value();
    }

    std::optional<Statement> MetadataTable::TryGetNamedValueStatement(const Connection& connection, std::string_view name, std::string_view database)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());

        Statement result = Statement::Create(connection, GetNamedValueSql(database));
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

    Statement MetadataTable::SetNamedValueStatement(const Connection& connection, std::string_view name)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());
        Statement result = Statement::Create(connection, s_MetadataTableStmt_SetNamedValue);
        result.Bind(1, name);
        return result;
    }
}
