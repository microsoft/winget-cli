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

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(result._dbconn, "sqliteindex_createnew");

        Schema::MetadataTable::Create(result._dbconn);
        // Use calculated version, as incoming version could be 'latest'
        result._version.SetSchemaVersion(result._dbconn);

        savepoint.Commit();

        return result;
    }

    SQLiteIndex SQLiteIndex::Open(const std::string& filePath, OpenDisposition disposition)
    {
        switch (disposition)
        {
        case AppInstaller::Repository::Microsoft::SQLiteIndex::OpenDisposition::Read:
            return { filePath, SQLite::Connection::OpenDisposition::ReadOnly, SQLite::Connection::OpenFlags::None };
        case AppInstaller::Repository::Microsoft::SQLiteIndex::OpenDisposition::ReadWrite:
            return { filePath, SQLite::Connection::OpenDisposition::ReadWrite, SQLite::Connection::OpenFlags::None };
        case AppInstaller::Repository::Microsoft::SQLiteIndex::OpenDisposition::Immutable:
        {
            // Following the algorithm set forth at https://sqlite.org/uri.html [3.1] to convert to a URI path
            // The execution order builds out the string so that it shouldn't require any moves (other than growing)
            std::string target;
            // Add an 'arbitrary' growth size to prevent the majority of needing to grow (adding 'file:/' and '?immutable=1')
            target.reserve(filePath.size() + 20);

            target += "file:";

            bool wasLastCharSlash = false;

            if (filePath.size() >= 2 && filePath[1] == ':' &&
                ((filePath[0] >= 'a' && filePath[0] <= 'z') ||
                 (filePath[0] >= 'A' && filePath[0] <= 'Z')))
            {
                target += '/';
                wasLastCharSlash = true;
            }

            for (char c : filePath)
            {
                bool wasThisCharSlash = false;
                switch (c)
                {
                case '?': target += "%3f"; break;
                case '#': target += "%23"; break;
                case '\\':
                case '/':
                {
                    wasThisCharSlash = true;
                    if (!wasLastCharSlash)
                    {
                        target += '/';
                    }
                    break;
                }
                default: target += c; break;
                }

                wasLastCharSlash = wasThisCharSlash;
            }

            target += "?immutable=1";

            return { target, SQLite::Connection::OpenDisposition::ReadOnly, SQLite::Connection::OpenFlags::Uri };
        }
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    SQLiteIndex::SQLiteIndex(const std::string& target, SQLite::Connection::OpenDisposition disposition, SQLite::Connection::OpenFlags flags) :
        _dbconn(SQLite::Connection::Create(target, disposition, flags))
    {
        _version = Schema::Version::GetSchemaVersion(_dbconn);
        _interface = _version.CreateISQLiteIndex();
    }

    SQLiteIndex::SQLiteIndex(const std::string& target, Schema::Version version) :
        _dbconn(SQLite::Connection::Create(target, SQLite::Connection::OpenDisposition::Create))
    {
        _interface = version.CreateISQLiteIndex();
        _version = _interface->GetVersion();
    }
}
