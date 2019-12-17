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

    SQLite::Statement MetadataTable::GetNamedValueStatement(const std::string& name)
    {

    }
}
