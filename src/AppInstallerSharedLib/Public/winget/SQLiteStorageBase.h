// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteWrapper.h>
#include <winget/SQLiteVersion.h>
#include <winget/ManagedFile.h>

#include <filesystem>
#include <mutex>
#include <vector>

namespace AppInstaller::SQLite
{
    // Type that wraps the basic SQLite storage functionality; the connection and metadata like schema version.
    struct SQLiteStorageBase
    {
        // The disposition for opening the database.
        using OpenDisposition = DatabaseDisposition;

        SQLiteStorageBase(SQLiteStorageBase&&) = default;
        SQLiteStorageBase& operator=(SQLiteStorageBase&& other) noexcept;

        // Gets the last write time for the database.
        std::chrono::system_clock::time_point GetLastWriteTime() const;

        // Gets the identifier written to the database when it was created.
        std::string GetDatabaseIdentifier() const;

        // Gets the schema version of the database.
        const Version& GetVersion() const { return m_version; }

        // Renames the database file and any auxiliary files given the inputs.
        // Should only be used on an inactive database.
        // If overwrite is given, existing destination files will be removed first.
        static void RenameSQLiteDatabase(const std::filesystem::path& source, const std::filesystem::path& destination, bool overwrite = false);

        // Deletes the database file and any auxiliary files without opening them.
        // Should only be used on an inactive database. Missing files are ignored.
        static void DeleteSQLiteDatabase(const std::filesystem::path& path);

    protected:
        SQLiteStorageBase(const std::string& target, const Version& version, size_t pageSize = 0);

        SQLiteStorageBase(const std::string& filePath, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& indexFile);

        SQLiteStorageBase(const DatabaseSpecifier& specifier, Utility::ManagedFile&& indexFile);

        SQLiteStorageBase(const DatabaseSpecifier& specifier, std::vector<Utility::ManagedFile>&& files);

        SQLiteStorageBase(const std::string& target, SQLiteStorageBase& source);

        // Sets the last write time metadata value in the database.
        void SetLastWriteTime();

        // Keep every backing file alive until after the connection (including attachments) closes.
        std::vector<Utility::ManagedFile> m_files;
        SQLite::Connection m_dbconn;
        Version m_version;
        std::unique_ptr<std::mutex> m_interfaceLock = std::make_unique<std::mutex>();
    };
}
