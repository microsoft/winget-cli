// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SQLiteIndex.h"

#include "Schema/MetadataTable.h"

namespace AppInstaller::Repository::Microsoft
{
    SQLiteIndex SQLiteIndex::CreateNew(const std::string& filePath, Schema::Version version)
    {
        SQLiteIndex result{ filePath, version };
        Schema::MetadataTable::Create(result._dbconn);
    }

    SQLiteIndex SQLiteIndex::Open(const std::string& filePath, OpenDisposition disposition)
    {

    }

    SQLiteIndex::SQLiteIndex(const std::string& target, SQLite::Connection::OpenDisposition disposition, SQLite::Connection::OpenFlags flags) :
        _dbconn(SQLite::Connection::Create(target, disposition, flags))
    {

    }

    SQLiteIndex::SQLiteIndex(const std::string& target, Schema::Version version) :
        _dbconn(SQLite::Connection::Create(target, SQLite::Connection::OpenDisposition::Create)),
        _version(version)
    {
        _interface = _version.CreateISQLiteIndex();
    }
}
