// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Version.h"
#include "MetadataTable.h"

namespace AppInstaller::Repository::Microsoft::Schema
{
    Version Version::GetSchemaVersion(SQLite::Connection& connection)
    {
        int major = MetadataTable::GetNamedValue<int>(connection, Schema::s_MetadataValueName_MajorVersion);
        int minor = MetadataTable::GetNamedValue<int>(connection, Schema::s_MetadataValueName_MinorVersion);

        return { static_cast<uint32_t>(major), static_cast<uint32_t>(minor) };
    }

    void Version::SetSchemaVersion(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "version_setschemaversion");

        MetadataTable::SetNamedValue(connection, Schema::s_MetadataValueName_MajorVersion, static_cast<int>(MajorVersion));
        MetadataTable::SetNamedValue(connection, Schema::s_MetadataValueName_MinorVersion, static_cast<int>(MinorVersion));

        savepoint.Commit();
    }

    std::ostream& operator<<(std::ostream& out, const Version& version)
    {
        return (out << version.MajorVersion << '.' << version.MinorVersion);
    }
}
