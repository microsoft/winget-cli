// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/ISQLiteIndex.h"

#include <limits>
#include <memory>

namespace AppInstaller::Repository::Microsoft::Schema
{
    // Forward declarations
    struct ISQLiteIndex;

    // Represents the schema version of the index.
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

        // Gets a version that represents the latest schema known to the implementation.
        static constexpr Version Latest() { return { std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max() }; }

        // Gets a version that represents the latest schema known to the implementation for the given major version.
        static constexpr Version LatestForMajor(uint32_t majorVersion) { return { majorVersion, std::numeric_limits<uint32_t>::max() }; }

        // Determines if this version represents the latest schema.
        bool IsLatest() const { return (MajorVersion == std::numeric_limits<uint32_t>::max() && MinorVersion == std::numeric_limits<uint32_t>::max()); }

        // Determines if this version represents the latest schema of the given major version.
        bool IsLatestForMajor(uint32_t majorVersion) const { return (MajorVersion == majorVersion && MinorVersion == std::numeric_limits<uint32_t>::max()); }

        // Determines the schema version of the opened index.
        static Version GetSchemaVersion(SQLite::Connection& connection);

        // Writes the current version to the given index.
        void SetSchemaVersion(SQLite::Connection& connection);

        // Creates the interface object for this version.
        std::unique_ptr<ISQLiteIndex> CreateISQLiteIndex() const;
    };

    // Output the version
    std::ostream& operator<<(std::ostream& out, const Version& version);
}
