// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MetadataTable.h"


namespace AppInstaller::Repository::Microsoft::Schema
{
    char const* const s_MetadataValueName_MajorVersion = "majorVersion";
    char const* const s_MetadataValueName_MinorVersion = "minorVersion";

    // Table data
    #define AIC_METADATA_TABLE_NAME "metadata"
    #define AIC_METADATA_TABLE_COLUMN_NAME "name"
    #define AIC_METADATA_TABLE_COLUMN_VALUE "value"

    // Statements
    char const* const s_MetadataTableStmt_GetNamedValue = "Select " AIC_METADATA_TABLE_COLUMN_VALUE;

    void MetadataTable::Create(SQLite::Connection& connection)
    {
        CREATE TABLE[main].[blah](
            [name] TEXT PRIMARY KEY NOT NULL UNIQUE,
            [value] TEXT NOT NULL);
    }

    SQLite::Statement MetadataTable::GetNamedValueStatement(SQLite::Connection& connection, const std::string& name)
    {

    }

    SQLite::Statement MetadataTable::SetNamedValueStatement(SQLite::Connection& connection, const std::string& name)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());
        SQLite::Statement result = SQLite::Statement::Create(connection, );
        result.Bind(1, name);
        return result;
    }
}
