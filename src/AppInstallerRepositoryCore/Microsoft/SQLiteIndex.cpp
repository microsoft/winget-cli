// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SQLiteIndex.h"

#include "Schema/MetadataTable.h"

namespace AppInstaller::Repository::Microsoft
{
    namespace
    {
        char const* const GetOpenDispositionString(SQLiteIndex::OpenDisposition disposition)
        {
            switch (disposition)
            {
            case AppInstaller::Repository::Microsoft::SQLiteIndex::OpenDisposition::Read:
                return "Read";
            case AppInstaller::Repository::Microsoft::SQLiteIndex::OpenDisposition::ReadWrite:
                return "ReadWrite";
            case AppInstaller::Repository::Microsoft::SQLiteIndex::OpenDisposition::Immutable:
                return "ImmutableRead";
            default:
                return "Unknown";
            }
        }
    }

    SQLiteIndex SQLiteIndex::CreateNew(const std::string& filePath, Schema::Version version)
    {
        AICLI_LOG(Repo, Info, << "Creating new SQLite Index [" << version << "] at '" << filePath << "'");
        SQLiteIndex result{ filePath, version };

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(result.m_dbconn, "sqliteindex_createnew");

        Schema::MetadataTable::Create(result.m_dbconn);
        // Use calculated version, as incoming version could be 'latest'
        result.m_version.SetSchemaVersion(result.m_dbconn);

        result.m_interface->CreateTables(result.m_dbconn);

        savepoint.Commit();

        return result;
    }

    SQLiteIndex SQLiteIndex::Open(const std::string& filePath, OpenDisposition disposition)
    {
        AICLI_LOG(Repo, Info, << "Opening SQLite Index for " << GetOpenDispositionString(disposition) << " at '" << filePath << "'");
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
        m_dbconn(SQLite::Connection::Create(target, disposition, flags))
    {
        m_version = Schema::Version::GetSchemaVersion(m_dbconn);
        AICLI_LOG(Repo, Info, << "Opened SQLite Index with version: " << m_version);
        m_interface = m_version.CreateISQLiteIndex();
    }

    SQLiteIndex::SQLiteIndex(const std::string& target, Schema::Version version) :
        m_dbconn(SQLite::Connection::Create(target, SQLite::Connection::OpenDisposition::Create))
    {
        m_interface = version.CreateISQLiteIndex();
        m_version = m_interface->GetVersion();
    }

    void SQLiteIndex::AddManifest(const std::filesystem::path& manifestPath, const std::filesystem::path& relativePath)
    {
        Manifest::Manifest manifest = Manifest::Manifest::CreateFromPath(manifestPath);
        AddManifest(manifest, relativePath);
    }

    void SQLiteIndex::AddManifest(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        m_interface->AddManifest(manifest, relativePath);
    }

    void SQLiteIndex::UpdateManifest(const std::filesystem::path& oldManifestPath, const std::filesystem::path& oldRelativePath, const std::filesystem::path& newManifestPath, const std::filesystem::path& newRelativePath)
    {
        Manifest::Manifest oldManifest = Manifest::Manifest::CreateFromPath(oldManifestPath);
        Manifest::Manifest newManifest = Manifest::Manifest::CreateFromPath(newManifestPath);
        UpdateManifest(oldManifest, oldRelativePath, newManifest, newRelativePath);
    }

    void SQLiteIndex::UpdateManifest(const Manifest::Manifest& oldManifest, const std::filesystem::path& oldRelativePath, const Manifest::Manifest& newManifest, const std::filesystem::path& newRelativePath)
    {
        m_interface->UpdateManifest(oldManifest, oldRelativePath, newManifest, newRelativePath);
    }

    void SQLiteIndex::RemoveManifest(const std::filesystem::path& manifestPath, const std::filesystem::path& relativePath)
    {
        Manifest::Manifest manifest = Manifest::Manifest::CreateFromPath(manifestPath);
        RemoveManifest(manifest, relativePath);
    }

    void SQLiteIndex::RemoveManifest(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        m_interface->RemoveManifest(manifest, relativePath);
    }
}
