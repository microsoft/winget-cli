// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SQLiteIndex.h"

#include "Schema/MetadataTable.h"

namespace AppInstaller::Repository::Microsoft
{
    namespace
    {
        // Determines the schema version of the opened index.
        SQLiteIndex::Version ReadSchemaVersion(SQLite::Connection& connection)
        {

        }

        // Creates the appropriate interface for operating on the index given the version.
        std::unique_ptr<ISQLiteIndex> CreateIndexInterfaceFromVersion(SQLiteIndex::Version version)
        {

        }
    }

    SQLiteIndex SQLiteIndex::CreateNew(const std::string& filePath, Version version)
    {

    }

    SQLiteIndex SQLiteIndex::Open(const std::string& filePath, OpenDisposition disposition)
    {

    }

    SQLiteIndex::SQLiteIndex(const std::string& target, SQLite::Connection::OpenDisposition disposition, SQLite::Connection::OpenFlags flags) :
        _dbconn(SQLite::Connection::Create(target, disposition, flags))
    {

    }

    SQLiteIndex::SQLiteIndex(const std::string& target, Version version) :
        _dbconn(SQLite::Connection::Create(target, SQLite::Connection::OpenDisposition::Create)),
        _version(version)
    {
        _interface = CreateIndexInterfaceFromVersion(_version);
    }
}
