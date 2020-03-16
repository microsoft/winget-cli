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

    void Version::SetSchemaVersion(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "version_setschemaversion");

        MetadataTable::SetNamedValue(connection, Schema::s_MetadataValueName_MajorVersion, static_cast<int>(MajorVersion));
        MetadataTable::SetNamedValue(connection, Schema::s_MetadataValueName_MinorVersion, static_cast<int>(MinorVersion));

        savepoint.Commit();
    }

    // Creates the interface object for this version.
    std::unique_ptr<ISQLiteIndex> Version::CreateISQLiteIndex()
    {
        if (*this == Version{ 1, 0 } ||
            this->MajorVersion == 1 ||
            this->IsLatest())
        {
            return std::make_unique<V1_0::Interface>();
        }

        // We do not have the capacity to operate on this schema version
        THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
    }
}

std::ostream& operator<<(std::ostream& out, const AppInstaller::Repository::Microsoft::Schema::Version& version)
{
    return (out << version.MajorVersion << '.' << version.MinorVersion);
}
