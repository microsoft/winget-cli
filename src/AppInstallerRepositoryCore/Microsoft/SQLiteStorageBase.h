// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/Version.h"
#include <winget/ManagedFile.h>
#include <AppInstallerVersions.h>

#include <mutex>

namespace AppInstaller::Repository::Microsoft
{
    struct SQLiteStorageBase
    {
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

        // Gets the last write time for the index.
        std::chrono::system_clock::time_point GetLastWriteTime();

        // Gets the identifier written to the database when it was created.
        std::string GetDatabaseIdentifier();

        // Gets the schema version of the database.
        Schema::Version GetVersion() const { return m_version; }

    protected:
        SQLiteStorageBase(const std::string& target, Schema::Version version);

        SQLiteStorageBase(const std::string& filePath, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& indexFile);

        SQLiteStorageBase(const std::string& target, SQLiteStorageBase& source);

        // Sets the last write time metadata value in the database.
        void SetLastWriteTime();

        // Gets the corresponding OpenFlags based on the disposition.
        SQLite::Connection::OpenFlags GetOpenFlags(SQLiteStorageBase::OpenDisposition disposition);

        Utility::ManagedFile m_indexFile;
        SQLite::Connection m_dbconn;
        Schema::Version m_version;
        std::unique_ptr<std::mutex> m_interfaceLock = std::make_unique<std::mutex>();
    };
}