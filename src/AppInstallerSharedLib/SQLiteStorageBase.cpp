// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/SQLiteStorageBase.h"
#include "Public/winget/SQLiteMetadataTable.h"
#include "AppInstallerDateTime.h"
#include "AppInstallerLanguageUtilities.h"

namespace AppInstaller::SQLite
{
    namespace
    {
        constexpr std::wstring_view s_SQLiteFileSuffixes[] = { L"", L"-journal", L"-wal", L"-shm" };

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

    SQLiteStorageBase& SQLiteStorageBase::operator=(SQLiteStorageBase&& other) noexcept
    {
        if (this != &other)
        {
            // Release the old connection before deleting any of its backing files.
            m_dbconn = std::move(other.m_dbconn);
            m_files = std::move(other.m_files);
            m_version = std::move(other.m_version);
            m_interfaceLock = std::move(other.m_interfaceLock);
        }

        return *this;
    }

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
        THROW_WIN32_IF(ERROR_FILE_NOT_FOUND, !std::filesystem::exists(source));
        THROW_WIN32_IF(ERROR_DIRECTORY, std::filesystem::is_directory(source));

        if (overwrite)
        {
            for (const auto& suffix : s_SQLiteFileSuffixes)
            {
                std::filesystem::path target = AddSuffix(destination, suffix);

                if (std::filesystem::exists(target))
                {
                    std::filesystem::remove_all(target);
                }
            }
        }

        for (const auto& suffix : s_SQLiteFileSuffixes)
        {
            std::filesystem::path target = AddSuffix(source, suffix);

            if (std::filesystem::exists(target))
            {
                std::filesystem::rename(target, AddSuffix(destination, suffix));
            }
        }
    }

    void SQLiteStorageBase::DeleteSQLiteDatabase(const std::filesystem::path& path)
    {
        THROW_HR_IF(E_INVALIDARG, path.empty());

        for (const auto& suffix : s_SQLiteFileSuffixes)
        {
            std::filesystem::remove(AddSuffix(path, suffix));
        }
    }

    SQLiteStorageBase::SQLiteStorageBase(const std::string& filePath, OpenDisposition disposition, Utility::ManagedFile&& file) :
        SQLiteStorageBase(DatabaseSpecifier{ filePath, disposition }, std::move(file))
    {
    }

    SQLiteStorageBase::SQLiteStorageBase(const DatabaseSpecifier& specifier, Utility::ManagedFile&& file) :
        SQLiteStorageBase(specifier, InitializeFromMoveOnly<std::vector<Utility::ManagedFile>>({ std::move(file) }))
    {
    }

    SQLiteStorageBase::SQLiteStorageBase(const DatabaseSpecifier& specifier, std::vector<Utility::ManagedFile>&& files) :
        m_files(std::move(files))
    {
        AICLI_LOG(Repo, Info, << "Opening database for " << GetOpenDispositionString(specifier.Disposition()) << " at '" << specifier.Path() << "'");

        m_dbconn = SQLite::Connection::Create(specifier);

        m_version = Version::GetSchemaVersion(m_dbconn);
    }

    SQLiteStorageBase::SQLiteStorageBase(const std::string& target, const Version& version, size_t pageSize) :
        m_dbconn(SQLite::Connection::Create(target, SQLite::Connection::OpenDisposition::Create))
    {
        m_version = version;
        if (pageSize > 0)
        {
            m_dbconn.SetPageSize(pageSize);
        }
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
