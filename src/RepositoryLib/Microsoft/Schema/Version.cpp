// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Version.h"
#include "MetadataTable.h"

#include "1_0/Interface.h"

namespace AppInstaller::Repository::Microsoft::Schema
{
    Version Version::GetSchemaVersion(SQLite::Connection& connection)
    {
        int major = MetadataTable::GetNamedValue<int>(connection, Schema::s_MetadataValueName_MajorVersion);
        int minor = MetadataTable::GetNamedValue<int>(connection, Schema::s_MetadataValueName_MinorVersion);

        return { static_cast<uint32_t>(major), static_cast<uint32_t>(minor) };
    }

    // Creates the interface object for this version.
    std::unique_ptr<ISQLiteIndex> Version::CreateISQLiteIndex()
    {
        if (*this == Version{ 1, 0 })
        {
            return std::make_unique<V1_0::Interface>();
        }
    }
}
