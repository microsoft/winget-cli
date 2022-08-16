// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PortableIndex.h"
#include "Schema/MetadataTable.h"

namespace AppInstaller::Repository::Microsoft
{
    namespace
    {
        char const* const GetOpenDispositionString(PortableIndex::OpenDisposition disposition)
        {
            switch (disposition)
            {
            case AppInstaller::Repository::Microsoft::PortableIndex::OpenDisposition::Read:
                return "Read";
            case AppInstaller::Repository::Microsoft::PortableIndex::OpenDisposition::ReadWrite:
                return "ReadWrite";
            case AppInstaller::Repository::Microsoft::PortableIndex::OpenDisposition::Immutable:
                return "ImmutableRead";
            default:
                return "Unknown";
            }
        }
    }

    PortableIndex PortableIndex::CreateNew(const std::string& filePath, Schema::Version version)
    {
        AICLI_LOG(Repo, Info, << "Creating new Portable Index [" << version << "] at '" << filePath << "'");
        PortableIndex result{ filePath, version };

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(result.m_dbconn, "portableindex_createnew");

        Schema::MetadataTable::Create(result.m_dbconn);

        // Use calculated version, as incoming version could be 'latest'
        result.m_version.SetSchemaVersion(result.m_dbconn);

        result.m_interface->CreateTable(result.m_dbconn);

        //result.SetLastWriteTime();

        savepoint.Commit();

        return result;
    }

    PortableIndex PortableIndex::Open(const std::string& filePath, OpenDisposition disposition, Utility::ManagedFile&& indexFile)
    {
        AICLI_LOG(Repo, Info, << "Opening SQLite Index for " << GetOpenDispositionString(disposition) << " at '" << filePath << "'");
        switch (disposition)
        {
        case AppInstaller::Repository::Microsoft::PortableIndex::OpenDisposition::Read:
            return { filePath, SQLite::Connection::OpenDisposition::ReadOnly, SQLite::Connection::OpenFlags::None, std::move(indexFile) };
        case AppInstaller::Repository::Microsoft::PortableIndex::OpenDisposition::ReadWrite:
            return { filePath, SQLite::Connection::OpenDisposition::ReadWrite, SQLite::Connection::OpenFlags::None, std::move(indexFile) };
        case AppInstaller::Repository::Microsoft::PortableIndex::OpenDisposition::Immutable:
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

    PortableIndex::IdType PortableIndex::AddPortableFile(const Schema::Portable_V1_0::PortableFile& file)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Adding portable file for [" << file.FilePath << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "portableindex_addfile");

        IdType result = m_interface->AddPortableFile(m_dbconn, file);

        //SetLastWriteTime();

        savepoint.Commit();

        return result;
    }

    void PortableIndex::RemovePortableFile(const Schema::Portable_V1_0::PortableFile& file)
    {
        AICLI_LOG(Repo, Verbose, << "Removing portable file [" << file.FilePath << "]");
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "portableindex_removemanifest");

        m_interface->RemovePortableFile(m_dbconn, file);

        //SetLastWriteTime();

        savepoint.Commit();
    }


    PortableIndex::PortableIndex(const std::string& target, SQLite::Connection::OpenDisposition disposition, SQLite::Connection::OpenFlags flags, Utility::ManagedFile&& indexFile) :
        m_dbconn(SQLite::Connection::Create(target, disposition, flags)), m_indexFile(std::move(indexFile))
    {
        m_dbconn.EnableICU();
        m_version = Schema::Version::GetSchemaVersion(m_dbconn);
        //AICLI_LOG(Repo, Info, << "Opened SQLite Index with version [" << m_version << "], last write [" << GetLastWriteTime() << "]");
        m_interface = m_version.CreateIPortableIndex();
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_CANNOT_WRITE_TO_UPLEVEL_INDEX, disposition == SQLite::Connection::OpenDisposition::ReadWrite && m_version != m_interface->GetVersion());
    }

    PortableIndex::PortableIndex(const std::string& target, Schema::Version version) :
        m_dbconn(SQLite::Connection::Create(target, SQLite::Connection::OpenDisposition::Create))
    {
        m_dbconn.EnableICU();
        m_interface = version.CreateIPortableIndex();
        m_version = m_interface->GetVersion();
    }
}