// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteWrapper.h>
#include <memory>

namespace AppInstaller::SQLite
{
    // Represents the schema version of the database.
    struct Version
    {
        // The major version of the schema.
        // All minor changes to this major version must be backward compatible.
        uint32_t MajorVersion{};
        // The minor version of the schema.
        // All changes to the schema warrant a change to the minor version.
        uint32_t MinorVersion{};

        bool operator==(const Version& other) const
        {
            return (MajorVersion == other.MajorVersion && MinorVersion == other.MinorVersion);
        }

        bool operator!=(const Version& other) const
        {
            return !operator==(other);
        }

        bool operator>=(const Version& other) const
        {
            if (MajorVersion > other.MajorVersion) return true;
            if (MajorVersion < other.MajorVersion) return false;
            return MinorVersion >= other.MinorVersion;
        }

        bool operator<(const Version& other) const
        {
            if (MajorVersion < other.MajorVersion) return true;
            if (MajorVersion > other.MajorVersion) return false;
            return MinorVersion < other.MinorVersion;
        }

        // Gets a version that represents the latest schema known to the implementation.
        static Version Latest();

        // Gets a version that represents the latest schema known to the implementation for the given major version.
        static Version LatestForMajor(uint32_t majorVersion);

        // Determines if this version represents the latest schema.
        bool IsLatest() const;

        // Determines if this version represents the latest schema of the given major version.
        bool IsLatestForMajor(uint32_t majorVersion) const;

        // Determines the schema version of the opened database.
        static Version GetSchemaVersion(Connection& connection);

        // Writes the current version to the given database.
        void SetSchemaVersion(Connection& connection) const;
    };

    // Output the version
    std::ostream& operator<<(std::ostream& out, const Version& version);
}
