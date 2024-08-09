// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/SQLiteVersion.h"
#include "Public/winget/SQLiteMetadataTable.h"

#include <limits>

namespace AppInstaller::SQLite
{
    Version Version::GetSchemaVersion(Connection& connection)
    {
        int major = MetadataTable::GetNamedValue<int>(connection, s_MetadataValueName_MajorVersion);
        int minor = MetadataTable::GetNamedValue<int>(connection, s_MetadataValueName_MinorVersion);

        return { static_cast<uint32_t>(major), static_cast<uint32_t>(minor) };
    }

    void Version::SetSchemaVersion(Connection& connection) const
    {
        Savepoint savepoint = Savepoint::Create(connection, "version_setschemaversion");

        MetadataTable::SetNamedValue(connection, s_MetadataValueName_MajorVersion, static_cast<int>(MajorVersion));
        MetadataTable::SetNamedValue(connection, s_MetadataValueName_MinorVersion, static_cast<int>(MinorVersion));

        savepoint.Commit();
    }

    std::ostream& operator<<(std::ostream& out, const Version& version)
    {
        if (version.IsLatest())
        {
            return out << "Latest";
        }
        else if (version.IsLatestForMajor(version.MajorVersion))
        {
            return out << version.MajorVersion << ".Latest";
        }
        else
        {
            return out << version.MajorVersion << '.' << version.MinorVersion;
        }
    }

    Version Version::Latest()
    {
        return { std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max() };
    }

    Version Version::LatestForMajor(uint32_t majorVersion)
    {
        return { majorVersion, std::numeric_limits<uint32_t>::max() };
    }

    bool Version::IsLatest() const
    {
        return (MajorVersion == std::numeric_limits<uint32_t>::max() && MinorVersion == std::numeric_limits<uint32_t>::max());
    }

    bool Version::IsLatestForMajor(uint32_t majorVersion) const
    {
        return (MajorVersion == majorVersion && MinorVersion == std::numeric_limits<uint32_t>::max());
    }
}
