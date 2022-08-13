// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PortableMetadataTable.h"


namespace AppInstaller::Repository::Microsoft::Schema
{
    // Table data [note that this table is not versioned, and thus *cannot change*]
    static constexpr std::string_view s_PortableMetadataTable_Table_Name = "portablemetadata"sv;
    static constexpr std::string_view s_PortableMetadataTable_FilePath_Column = "filepath"sv;
    static constexpr std::string_view s_PortableMetadataTable_FileType_Column = "filetype"sv;
    static constexpr std::string_view s_PortableMetadataTable_Sha256_Column = "sha256"sv;
    static constexpr std::string_view s_PortableMetadataTable_SymlinkTarget_Column = "symlinkTarget"sv;
    static constexpr std::string_view s_PortableMetadataTable_IsCreated_Column = "iscreated"sv;

    static constexpr std::string_view s_PortableMetadataTable_Table_Create = R"(
CREATE TABLE [portablemetadata](
    [filepath] TEXT PRIMARY KEY NOT NULL,
    [filetype] TEXT NOT NULL)
)"sv;

    // Statements
    static constexpr std::string_view s_PortableMetadataTableStmt_GetNamedValue = "select [value] from [metadata] where [name] = ?"sv;
    static constexpr std::string_view s_PortableMetadataTableStmt_SetNamedValue = "insert or replace into [portablemetadata] ([filepath], [filetype]) values (?, ?)"sv;

    void PortableMetadataTable::Create(SQLite::Connection& connection)
    {
        SQLite::Statement create = SQLite::Statement::Create(connection, s_PortableMetadataTable_Table_Create);
        create.Execute();
    }

    SQLite::Statement PortableMetadataTable::GetNamedValueStatement(SQLite::Connection& connection, std::string_view name)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());
        SQLite::Statement result = SQLite::Statement::Create(connection, s_PortableMetadataTableStmt_GetNamedValue);
        result.Bind(1, name);
        THROW_HR_IF(E_NOT_SET, !result.Step());
        return result;
    }

    SQLite::Statement PortableMetadataTable::SetNamedValueStatement(SQLite::Connection& connection, std::string_view name)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());
        SQLite::Statement result = SQLite::Statement::Create(connection, s_PortableMetadataTableStmt_SetNamedValue);
        result.Bind(1, name);
        return result;
    }
}