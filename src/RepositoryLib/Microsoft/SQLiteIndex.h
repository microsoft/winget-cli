// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"

#include <limits>
#include <memory>
#include <string>

namespace AppInstaller::Repository::Microsoft
{
    // Forward declaration of internals
    struct ISQLiteIndex;

    // Holds the connection to the database, as well as the appropriate functionality to interface with it.
    struct SQLiteIndex
    {
        // Represents the schema version of the index.
        struct Version
        {
            // The major version of the schema.
            // All minor changes to this major version must be backward compatible.
            uint32_t MajorVersion{};
            // The minor version of the schema.
            // All changes to the schema warrant a change to the minor version.
            uint32_t MinorVersion{};

            // Gets a version that represents the latest schema known to the implementation.
            static constexpr Version Latest() { return { std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max() }; }

            // Determines if this version represents the latest schema.
            bool IsLatest() const { return (MajorVersion == std::numeric_limits<uint32_t>::max() && MinorVersion == std::numeric_limits<uint32_t>::max()); }
        };

        // Creates a new index database of the given version.
        static SQLiteIndex CreateNew(const std::string& filePath, Version version);

        // The disposition for opening the index.
        enum class OpenDisposition
        {
            // Open for read only.
            Read,
            // Open for read and write.
            ReadWrite,
            // The database will not change while in use; open for immutable read.
            Immutable,
        };

        // Opens an existing index database.
        static SQLiteIndex Open(const std::string& filePath, OpenDisposition disposition);

    private:
        // Constructor used to open an existing index.
        SQLiteIndex(const std::string& target, SQLite::Connection::OpenDisposition disposition, SQLite::Connection::OpenFlags flags);

        // Constructor used to create a new index.
        SQLiteIndex(const std::string& target, Version version);

        SQLite::Connection _dbconn;
        Version _version;
        std::unique_ptr<ISQLiteIndex> _interface;
    };
}
