// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MetadataTable.h"


namespace AppInstaller::Repository::Microsoft::Schema
{
    char const* const s_MetadataValueName_MajorVersion = "majorVersion";
    char const* const s_MetadataValueName_MinorVersion = "minorVersion";

    void MetadataTable::Create(SQLite::Connection& connection)
    {

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
