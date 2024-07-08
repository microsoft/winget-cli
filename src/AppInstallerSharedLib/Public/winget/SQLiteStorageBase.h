// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteWrapper.h>
#include <winget/SQLiteVersion.h>
#include <winget/ManagedFile.h>

#include <mutex>

namespace AppInstaller::SQLite
{
    // Type that wraps the basic SQLite storage functionality; the connection and metadata like schema version.
    struct SQLiteStorageBase
    {
        // The disposition for opening the database.
        enum class OpenDisposition
        {
            // Open for read only.
            Read,
            // Open for read and write.
            ReadWrite,
            // The database will not change while in use; open for immutable read.
            Immutable,
        };

        // Gets the last write time for the database.
        std::chrono::system_clock::time_point GetLastWriteTime() const;

        // Gets the identifier written to the database when it was created.
        std::string GetDatabaseIdentifier() const;

        // Gets the schema version of the database.
        const Version& GetVersion() const { return m_version; }

    protected:
        SQLiteStorageBase(const std::string& target, const Version& version);

        SQLiteStorageBase(const std::string& filePath, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& indexFile);

        SQLiteStorageBase(const std::string& target, SQLiteStorageBase& source);

        // Sets the last write time metadata value in the database.
        void SetLastWriteTime();

        Utility::ManagedFile m_indexFile;
        SQLite::Connection m_dbconn;
        Version m_version;
        std::unique_ptr<std::mutex> m_interfaceLock = std::make_unique<std::mutex>();
    };
}
