// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/Version.h"
#include "Portable_1_0/PortableTable.h"
#include <AppInstallerVersions.h>

namespace AppInstaller::Repository::Microsoft::Schema
{
    // Forward declarations
    struct Version;

    struct IPortableIndex
    {
        virtual ~IPortableIndex() = default;

        // Version 1.0

        // Gets the schema version that this index interface is built for.
        virtual Schema::Version GetVersion() const = 0;

        // Creates all of the version dependent tables within the database.
        virtual void CreateTable(SQLite::Connection& connection) = 0;

        // Adds a portable entry to the index.
        virtual SQLite::rowid_t AddPortableFile(SQLite::Connection& connection, const Portable_V1_0::PortableFile& file) = 0;

        // Removes a portable entry from the index.
        virtual SQLite::rowid_t RemovePortableFile(SQLite::Connection& connection, const Portable_V1_0::PortableFile& file) = 0;

        // Updates the entry with matching { FileType, SHA256 } in the index.
        // The return value indicates whether the index was modified by the function.
        //virtual std::pair<bool, SQLite::rowid_t> UpdatePortableFile(SQLite::Connection& connection, const Portable_V1_0::PortableFile& file) = 0;
    };
}
