// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SQLiteStorageBase.h"
#include "Schema/MetadataTable.h"

namespace AppInstaller::Repository::Microsoft
{
    namespace
    {
        static char const* const GetOpenDispositionString(SQLiteStorageBase::OpenDisposition disposition)
        {
            switch (disposition)
            {
            case AppInstaller::Repository::Microsoft::SQLiteStorageBase::OpenDisposition::Read:
                return "Read";
            case AppInstaller::Repository::Microsoft::SQLiteStorageBase::OpenDisposition::ReadWrite:
                return "ReadWrite";
            case AppInstaller::Repository::Microsoft::SQLiteStorageBase::OpenDisposition::Immutable:
                return "ImmutableRead";
            default:
                return "Unknown";
            }
        }

        static std::string ConvertToUri(const std::string& filePath)
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
            return target;
        }

        static SQLite::Connection::OpenDisposition GetOpenDisposition(SQLiteStorageBase::OpenDisposition disposition)
        {
            switch (disposition)
            {
            case AppInstaller::Repository::Microsoft::SQLiteStorageBase::OpenDisposition::ReadWrite:
                return SQLite::Connection::OpenDisposition::ReadWrite;
            case AppInstaller::Repository::Microsoft::SQLiteStorageBase::OpenDisposition::Read:
            case AppInstaller::Repository::Microsoft::SQLiteStorageBase::OpenDisposition::Immutable:
            default:
                return SQLite::Connection::OpenDisposition::ReadOnly;
            }
        }
    }

    // One method for converting open disposition to proper open disposition
    // another method for obtaining the right flags
    void SQLiteStorageBase::SetLastWriteTime()
    {
        Schema::MetadataTable::SetNamedValue(m_dbconn, Schema::s_MetadataValueName_LastWriteTime, Utility::GetCurrentUnixEpoch());
    }

    SQLite::Connection::OpenFlags SQLiteStorageBase::GetOpenFlags(SQLiteStorageBase::OpenDisposition disposition)
    {
        switch (disposition)
        {
        case AppInstaller::Repository::Microsoft::SQLiteStorageBase::OpenDisposition::Immutable:
            return SQLite::Connection::OpenFlags::Uri;
        case AppInstaller::Repository::Microsoft::SQLiteStorageBase::OpenDisposition::Read:
        case AppInstaller::Repository::Microsoft::SQLiteStorageBase::OpenDisposition::ReadWrite:
        default:
            return SQLite::Connection::OpenFlags::None;
        }
    }

    // Recording last write time based on MSDN documentation stating that time returns a POSIX epoch time and thus
    // should be consistent across systems.
    std::chrono::system_clock::time_point SQLiteStorageBase::GetLastWriteTime()
    {
        int64_t lastWriteTime = Schema::MetadataTable::GetNamedValue<int64_t>(m_dbconn, Schema::s_MetadataValueName_LastWriteTime);
        return Utility::ConvertUnixEpochToSystemClock(lastWriteTime);
    }

    SQLiteStorageBase::SQLiteStorageBase(const std::string& target, OpenDisposition disposition, SQLite::Connection::OpenFlags flags, Utility::ManagedFile&& indexFile) :
        m_indexFile(std::move(indexFile))
    {
        AICLI_LOG(Repo, Info, << "Opening SQLite Index for " << GetOpenDispositionString(disposition) << " at '" << target << "'");
        const std::string& connectionTarget = (flags == SQLite::Connection::OpenFlags::Uri) ? ConvertToUri(target) : target;
        m_dbconn = SQLite::Connection::Create(connectionTarget, GetOpenDisposition(disposition), flags);
        m_version = Schema::Version::GetSchemaVersion(m_dbconn);
    }

    SQLiteStorageBase::SQLiteStorageBase(const std::string& target, Schema::Version version) :
        m_dbconn(SQLite::Connection::Create(target, SQLite::Connection::OpenDisposition::Create))
    {
        m_version = version;
        Schema::MetadataTable::Create(m_dbconn);
    }
}