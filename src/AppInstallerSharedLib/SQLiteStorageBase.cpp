// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/SQLiteStorageBase.h"
#include "Public/winget/SQLiteMetadataTable.h"
#include "AppInstallerDateTime.h"

namespace AppInstaller::SQLite
{
    namespace
    {
        static char const* const GetOpenDispositionString(SQLiteStorageBase::OpenDisposition disposition)
        {
            switch (disposition)
            {
            case SQLiteStorageBase::OpenDisposition::Read:
                return "Read";
            case SQLiteStorageBase::OpenDisposition::ReadWrite:
                return "ReadWrite";
            case SQLiteStorageBase::OpenDisposition::Immutable:
                return "ImmutableRead";
            default:
                return "Unknown";
            }
        }

        std::filesystem::path AddSuffix(const std::filesystem::path& source, std::wstring_view suffix)
        {
            std::filesystem::path result{ source };

            if (!suffix.empty())
            {
                std::wstring filename = result.filename().wstring();
                filename += suffix;
                result.replace_filename(std::move(filename));
            }

            return result;
        }
    }

    // One method for converting open disposition to proper open disposition
    // another method for obtaining the right flags
    void SQLiteStorageBase::SetLastWriteTime()
    {
        MetadataTable::SetNamedValue(m_dbconn, s_MetadataValueName_LastWriteTime, Utility::GetCurrentUnixEpoch());
    }

    // Recording last write time based on MSDN documentation stating that time returns a POSIX epoch time and thus
    // should be consistent across systems.
    std::chrono::system_clock::time_point SQLiteStorageBase::GetLastWriteTime() const
    {
        int64_t lastWriteTime = MetadataTable::GetNamedValue<int64_t>(m_dbconn, s_MetadataValueName_LastWriteTime);
        return Utility::ConvertUnixEpochToSystemClock(lastWriteTime);
    }

    std::string SQLiteStorageBase::GetDatabaseIdentifier() const
    {
        return MetadataTable::TryGetNamedValue<std::string>(m_dbconn, s_MetadataValueName_DatabaseIdentifier).value_or(std::string{});
    }

    void SQLiteStorageBase::RenameSQLiteDatabase(const std::filesystem::path& source, const std::filesystem::path& destination, bool overwrite)
    {
        auto fileSuffixes = { L"", L"-journal", L"-wal" };

        THROW_WIN32_IF(ERROR_FILE_NOT_FOUND, !std::filesystem::exists(source));
        THROW_WIN32_IF(ERROR_DIRECTORY, std::filesystem::is_directory(source));

        if (overwrite)
        {
            for (const auto& suffix : fileSuffixes)
            {
                std::filesystem::path target = AddSuffix(destination, suffix);

                if (std::filesystem::exists(target))
                {
                    std::filesystem::remove_all(target);
                }
            }
        }

        for (const auto& suffix : fileSuffixes)
        {
            std::filesystem::path target = AddSuffix(source, suffix);

            if (std::filesystem::exists(target))
            {
                std::filesystem::rename(target, AddSuffix(destination, suffix));
            }
        }
    }

    SQLiteStorageBase::SQLiteStorageBase(const std::string& filePath, OpenDisposition disposition, Utility::ManagedFile&& file) :
        m_indexFile(std::move(file))
    {
        AICLI_LOG(Repo, Info, << "Opening database for " << GetOpenDispositionString(disposition) << " at '" << filePath << "'");
        switch (disposition)
        {
        case OpenDisposition::Read:
            m_dbconn = SQLite::Connection::Create(filePath, SQLite::Connection::OpenDisposition::ReadOnly, SQLite::Connection::OpenFlags::None);
            break;
        case OpenDisposition::ReadWrite:
            m_dbconn = SQLite::Connection::Create(filePath, SQLite::Connection::OpenDisposition::ReadWrite, SQLite::Connection::OpenFlags::None);
            break;
        case OpenDisposition::Immutable:
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
            m_dbconn = SQLite::Connection::Create(filePath, SQLite::Connection::OpenDisposition::ReadOnly, SQLite::Connection::OpenFlags::Uri);
            break;
        }
        default:
            THROW_HR(E_UNEXPECTED);
        }

        m_version = Version::GetSchemaVersion(m_dbconn);
    }

    SQLiteStorageBase::SQLiteStorageBase(const std::string& target, const Version& version) :
        m_dbconn(SQLite::Connection::Create(target, SQLite::Connection::OpenDisposition::Create))
    {
        m_version = version;
        MetadataTable::Create(m_dbconn);

        // Write a new identifier for this database
        GUID databaseIdentifier;
        THROW_IF_FAILED(CoCreateGuid(&databaseIdentifier));
        std::ostringstream stream;
        stream << databaseIdentifier;
        MetadataTable::SetNamedValue(m_dbconn, s_MetadataValueName_DatabaseIdentifier, stream.str());
    }
    
    SQLiteStorageBase::SQLiteStorageBase(const std::string& target, SQLiteStorageBase& source) :
        m_dbconn(SQLite::Connection::Create(target, SQLite::Connection::OpenDisposition::Create)),
        m_version(source.m_version)
    {
        std::string mainDatabase = "main";
        Backup backup = Backup::Create(m_dbconn, mainDatabase, source.m_dbconn, mainDatabase);
        backup.Step();
    }
}
