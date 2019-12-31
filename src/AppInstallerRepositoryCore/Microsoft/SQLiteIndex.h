// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/Version.h"

#include <limits>
#include <memory>
#include <string>

namespace AppInstaller::Repository::Microsoft
{
    // Holds the connection to the database, as well as the appropriate functionality to interface with it.
    struct SQLiteIndex
    {
        SQLiteIndex(const SQLiteIndex&) = delete;
        SQLiteIndex& operator=(const SQLiteIndex&) = delete;

        SQLiteIndex(SQLiteIndex&&) = default;
        SQLiteIndex& operator=(SQLiteIndex&&) = default;

        // Creates a new index database of the given version.
        static SQLiteIndex CreateNew(const std::string& filePath, Schema::Version version);

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

        // Gets the schema version of the index.
        Schema::Version GetVersion() const { return m_version; }

    private:
        // Constructor used to open an existing index.
        SQLiteIndex(const std::string& target, SQLite::Connection::OpenDisposition disposition, SQLite::Connection::OpenFlags flags);

        // Constructor used to create a new index.
        SQLiteIndex(const std::string& target, Schema::Version version);

        SQLite::Connection m_dbconn;
        Schema::Version m_version;
        std::unique_ptr<Schema::ISQLiteIndex> m_interface;
    };
}
