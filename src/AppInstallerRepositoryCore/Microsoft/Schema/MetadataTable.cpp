// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MetadataTable.h"


namespace AppInstaller::Repository::Microsoft::Schema
{
    char const* const s_MetadataValueName_MajorVersion = "majorVersion";
    char const* const s_MetadataValueName_MinorVersion = "minorVersion";

    // Table data [note that this table is not versioned, and thus *cannot change*]
    static char const* const s_MetadataTable_Table_Name = "metadata";
    static char const* const s_MetadataTable_Column_Name = "name";
    static char const* const s_MetadataTable_Column_Value = "value";

    static char const* const s_MetadataTable_Table_Create = R"(
CREATE TABLE [metadata](
    [name] TEXT PRIMARY KEY NOT NULL,
    [value] TEXT NOT NULL)
)";

    // Statements
    static char const* const s_MetadataTableStmt_GetNamedValue = "select [value] from [metadata] where [name] = ?";
    static char const* const s_MetadataTableStmt_SetNamedValue = "insert or replace into [metadata] ([name], [value]) values (?, ?)";

    void MetadataTable::Create(SQLite::Connection& connection)
    {
        SQLite::Statement create = SQLite::Statement::Create(connection, s_MetadataTable_Table_Create);
        create.Execute();
    }

    SQLite::Statement MetadataTable::GetNamedValueStatement(SQLite::Connection& connection, const std::string& name)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());
        SQLite::Statement result = SQLite::Statement::Create(connection, s_MetadataTableStmt_GetNamedValue);
        result.Bind(1, name);
        THROW_HR_IF(E_NOT_SET, !result.Step());
        return result;
    }

    SQLite::Statement MetadataTable::SetNamedValueStatement(SQLite::Connection& connection, const std::string& name)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());
        SQLite::Statement result = SQLite::Statement::Create(connection, s_MetadataTableStmt_SetNamedValue);
        result.Bind(1, name);
        return result;
    }
}
