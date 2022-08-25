// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PortableIndex.h"
#include "SQLiteStorageBase.h"
#include "Schema/Portable_1_0/PortableIndexInterface.h"

namespace AppInstaller::Repository::Microsoft
{
    PortableIndex PortableIndex::CreateNew(const std::string& filePath, Schema::Version version)
    {
        AICLI_LOG(Repo, Info, << "Creating new Portable Index [" << version << "] at '" << filePath << "'");
        PortableIndex result{ filePath, version };

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(result.m_dbconn, "portableindex_createnew");

        // Use calculated version, as incoming version could be 'latest'
        result.m_version.SetSchemaVersion(result.m_dbconn);

        result.m_interface->CreateTable(result.m_dbconn);

        result.SetLastWriteTime();

        savepoint.Commit();

        return result;
    }

    PortableIndex::IdType PortableIndex::AddPortableFile(const Schema::IPortableIndex::PortableFile& file)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Adding portable file for [" << file.GetFilePath() << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "portableindex_addfile");

        IdType result = m_interface->AddPortableFile(m_dbconn, file);

        SetLastWriteTime();

        savepoint.Commit();

        return result;
    }

    void PortableIndex::RemovePortableFile(const Schema::IPortableIndex::PortableFile& file)
    {
        AICLI_LOG(Repo, Verbose, << "Removing portable file [" << file.GetFilePath() << "]");
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "portableindex_removefile");

        m_interface->RemovePortableFile(m_dbconn, file);

        SetLastWriteTime();

        savepoint.Commit();
    }

    bool PortableIndex::UpdatePortableFile(const Schema::IPortableIndex::PortableFile& file)
    {
        AICLI_LOG(Repo, Verbose, << "Updating portable file [" << file.GetFilePath() << "]");
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "portableindex_updatefile");

        bool result = m_interface->UpdatePortableFile(m_dbconn, file).first;
        
        if (result)
        {
            SetLastWriteTime();
            savepoint.Commit();
        }

        return result;
    }

    std::unique_ptr<Schema::IPortableIndex> PortableIndex::CreateIPortableIndex() const
    {
        if (m_version == Schema::Version{ 1, 0 } ||
            m_version.MajorVersion == 1 ||
            m_version.IsLatest())
        {
            return std::make_unique<Schema::Portable_V1_0::PortableIndexInterface>();
        }

        THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
    }

    PortableIndex::PortableIndex(const std::string& target, SQLiteStorageBase::OpenDisposition disposition, SQLite::Connection::OpenFlags flags, Utility::ManagedFile&& indexFile) :
        SQLiteStorageBase(target, disposition, flags, std::move(indexFile))
    {
        AICLI_LOG(Repo, Info, << "Opened Portable Index with version [" << m_version << "], last write [" << GetLastWriteTime() << "]");
        m_interface = CreateIPortableIndex();
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_CANNOT_WRITE_TO_UPLEVEL_INDEX, disposition == SQLiteStorageBase::OpenDisposition::ReadWrite && m_version != m_interface->GetVersion());
    }

    PortableIndex::PortableIndex(const std::string& target, Schema::Version version) : SQLiteStorageBase(target, version)
    {
        m_interface = CreateIPortableIndex();
        m_version = m_interface->GetVersion();
    }
}