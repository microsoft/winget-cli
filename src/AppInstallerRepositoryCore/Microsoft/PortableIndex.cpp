// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/PortableIndex.h"
#include "Microsoft/Schema/IPortableIndex.h"
#include "Microsoft/Schema/Portable_1_0/PortableTable.h"
#include <winget/SQLiteStorageBase.h>
#include "Schema/Portable_1_0/PortableIndexInterface.h"
#include <winget/Filesystem.h>

namespace AppInstaller::Repository::Microsoft
{
    PortableIndex::PortableIndex(PortableIndex&&) = default;
    PortableIndex& PortableIndex::operator=(PortableIndex&&) = default;

    PortableIndex::~PortableIndex() = default;

    PortableIndex PortableIndex::CreateNew(const std::string& filePath, SQLite::Version version)
    {
        AICLI_LOG(Repo, Info, << "Creating new Portable Index with version [" << version << "] at '" << filePath << "'");
        PortableIndex result{ filePath, version };

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(result.m_dbconn, "portableindex_createnew");

        // Use calculated version, as incoming version could be 'latest'
        result.m_version.SetSchemaVersion(result.m_dbconn);

        result.m_interface->CreateTable(result.m_dbconn);

        const auto& filePathUTF16 = Utility::ConvertToUTF16(filePath);
        SetFileAttributes(filePathUTF16.c_str(), GetFileAttributes(filePathUTF16.c_str()) | FILE_ATTRIBUTE_HIDDEN);

        result.SetLastWriteTime();

        savepoint.Commit();

        return result;
    }

    PortableIndex PortableIndex::Open(const std::string& filePath, OpenDisposition disposition, Utility::ManagedFile&& indexFile)
    {
        return { filePath, disposition, std::move(indexFile) };
    }

    PortableIndex::IdType PortableIndex::AddPortableFile(const Portable::PortableFileEntry& file)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Adding portable file for [" << file.GetFilePath() << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "portableindex_addfile");

        IdType result = m_interface->AddPortableFile(m_dbconn, file);

        SetLastWriteTime();

        savepoint.Commit();

        return result;
    }

    void PortableIndex::RemovePortableFile(const Portable::PortableFileEntry& file)
    {
        AICLI_LOG(Repo, Verbose, << "Removing portable file [" << file.GetFilePath() << "]");
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "portableindex_removefile");

        m_interface->RemovePortableFile(m_dbconn, file);

        SetLastWriteTime();

        savepoint.Commit();
    }

    bool PortableIndex::UpdatePortableFile(const Portable::PortableFileEntry& file)
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

    bool PortableIndex::Exists(const Portable::PortableFileEntry& file)
    {
        AICLI_LOG(Repo, Verbose, << "Checking if portable file exists [" << file.GetFilePath() << "]");
        return m_interface->Exists(m_dbconn, file);
    }

    bool PortableIndex::IsEmpty()
    {
        return m_interface->IsEmpty(m_dbconn);
    }

    void PortableIndex::AddOrUpdatePortableFile(const Portable::PortableFileEntry& file)
    {
        if (Exists(file))
        {
            UpdatePortableFile(file);
        }
        else
        {
            AddPortableFile(file);
        }
    }

    std::vector<Portable::PortableFileEntry> PortableIndex::GetAllPortableFiles()
    {
        return m_interface->GetAllPortableFiles(m_dbconn);
    }

    std::unique_ptr<Schema::IPortableIndex> PortableIndex::CreateIPortableIndex() const
    {
        if (m_version == SQLite::Version{ 1, 0 } ||
            m_version.MajorVersion == 1 ||
            m_version.IsLatest())
        {
            return std::make_unique<Schema::Portable_V1_0::PortableIndexInterface>();
        }

        THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
    }

    PortableIndex::PortableIndex(const std::string& target, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& indexFile) :
        SQLiteStorageBase(target, disposition, std::move(indexFile))
    {
        AICLI_LOG(Repo, Info, << "Opened Portable Index with version [" << m_version << "], last write [" << GetLastWriteTime() << "]");
        m_interface = CreateIPortableIndex();
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_CANNOT_WRITE_TO_UPLEVEL_INDEX, disposition == SQLiteStorageBase::OpenDisposition::ReadWrite && m_version != m_interface->GetVersion());
    }

    PortableIndex::PortableIndex(const std::string& target, SQLite::Version version) : SQLiteStorageBase(target, version)
    {
        m_interface = CreateIPortableIndex();
        m_version = m_interface->GetVersion();
    }
}