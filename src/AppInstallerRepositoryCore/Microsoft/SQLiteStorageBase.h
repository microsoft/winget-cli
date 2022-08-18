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

        template<typename T>
        static T Open(const std::string& filePath, OpenDisposition disposition, Utility::ManagedFile&& indexFile = {})
        {
            AICLI_LOG(Repo, Info, << "Opening SQLite Index for " << GetOpenDispositionString(disposition) << " at '" << filePath << "'");
            switch (disposition)
            {
            case OpenDisposition::Read:
                return { filePath, SQLite::Connection::OpenDisposition::ReadOnly, SQLite::Connection::OpenFlags::None, std::move(indexFile) };
            case OpenDisposition::ReadWrite:
                return { filePath, SQLite::Connection::OpenDisposition::ReadWrite, SQLite::Connection::OpenFlags::None, std::move(indexFile) };
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

                return { target, SQLite::Connection::OpenDisposition::ReadOnly, SQLite::Connection::OpenFlags::Uri, std::move(indexFile) };
            }
            default:
                THROW_HR(E_UNEXPECTED);
            }
        }

        // Gets the last write time for the index.
        std::chrono::system_clock::time_point GetLastWriteTime();

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