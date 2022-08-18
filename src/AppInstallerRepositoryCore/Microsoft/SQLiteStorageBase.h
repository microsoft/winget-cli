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
        enum class CreateOptions
        {
            // Standard
            None = 0x0,
            // Enable support for passing in nullopt values to Add/UpdateManifest
            SupportPathless = 0x1,
            // Disable support for dependencies
            DisableDependenciesSupport = 0x2,
        };

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

        template<typename T>
        static T Open(const std::string& filePath, OpenDisposition disposition, Utility::ManagedFile&& indexFile = {});

        // Gets the schema version of the index.
        Schema::Version GetVersion() const { return m_version; }

    protected:
        SQLiteStorageBase(const std::string& target, SQLite::Connection::OpenDisposition disposition, SQLite::Connection::OpenFlags flags, Utility::ManagedFile&& indexFile);

        SQLiteStorageBase(const std::string& target, Schema::Version version);

        // Sets the last write time metadata value in the index.
        void SetLastWriteTime();

        Utility::ManagedFile m_indexFile;
        SQLite::Connection m_dbconn;
        Schema::Version m_version;
        std::unique_ptr<std::mutex> m_interfaceLock = std::make_unique<std::mutex>();
    };
}