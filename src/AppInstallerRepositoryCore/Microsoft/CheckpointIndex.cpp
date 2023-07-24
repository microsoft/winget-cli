// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CheckpointIndex.h"
#include "SQLiteStorageBase.h"
#include "Schema/Checkpoint_1_0/CheckpointIndexInterface.h"

namespace AppInstaller::Repository::Microsoft
{
    CheckpointIndex CheckpointIndex::CreateNew(const std::string& filePath, Schema::Version version)
    {
        AICLI_LOG(Repo, Info, << "Creating new Savepoint Index with version [" << version << "] at '" << filePath << "'");
        CheckpointIndex result{ filePath, version };

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(result.m_dbconn, "CheckpointIndex_createnew");

        // Use calculated version, as incoming version could be 'latest'
        result.m_version.SetSchemaVersion(result.m_dbconn);

        result.m_interface->CreateTables(result.m_dbconn);

        result.SetLastWriteTime();

        savepoint.Commit();

        return result;
    }

    std::shared_ptr<CheckpointIndex> CheckpointIndex::OpenOrCreateDefault(GUID guid, OpenDisposition openDisposition)
    {
        wchar_t buffer[256];
        if (!StringFromGUID2(guid, buffer, ARRAYSIZE(buffer)))
        {
            THROW_HR(E_UNEXPECTED);
        }
        
        auto indexPath = Runtime::GetPathTo(Runtime::PathName::LocalState) / buffer;
        indexPath.replace_extension(".db");
        AICLI_LOG(Repo, Info, << "Opening savepoint index");

        try
        {
            if (std::filesystem::exists(indexPath))
            {
                if (std::filesystem::is_regular_file(indexPath))
                {
                    try
                    {
                        AICLI_LOG(Repo, Info, << "Opening existing savepoint index");
                        return std::make_shared<CheckpointIndex>(CheckpointIndex::Open(indexPath.u8string(), openDisposition));
                    }
                    CATCH_LOG();
                }

                AICLI_LOG(Repo, Info, << "Attempting to delete bad index file");
                std::filesystem::remove_all(indexPath);
            }

            return std::make_shared<CheckpointIndex>(CheckpointIndex::CreateNew(indexPath.u8string()));
        }
        CATCH_LOG();

        return {};
    }

    CheckpointIndex::IdType CheckpointIndex::AddCommandArgument(int type, const std::string_view& argValue)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Adding argument for [" << type << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "checkpointindex_addargument");

        IdType result = m_interface->AddCommandArgument(m_dbconn, type, argValue);

        SetLastWriteTime();

        savepoint.Commit();

        return result;
    }

    CheckpointIndex::IdType CheckpointIndex::SetClientVersion(std::string_view clientVersion)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Setting client version [" << clientVersion << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "checkpointindex_setclientversion");

        IdType result = m_interface->SetClientVersion(m_dbconn, clientVersion);

        SetLastWriteTime();

        savepoint.Commit();

        return result;
    }

    std::string CheckpointIndex::GetClientVersion()
    {
        return m_interface->GetClientVersion(m_dbconn);
    }

    CheckpointIndex::IdType CheckpointIndex::SetCommandName(std::string_view commandName)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Setting command name [" << commandName << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "checkpointindex_setcommandname");

        IdType result = m_interface->SetCommandName(m_dbconn, commandName);

        SetLastWriteTime();

        savepoint.Commit();

        return result;
    }

    std::string CheckpointIndex::GetCommandName()
    {
        return m_interface->GetCommandName(m_dbconn);
    }

    std::vector<std::pair<int, std::string>> CheckpointIndex::GetArguments()
    {
        return m_interface->GetArguments(m_dbconn);
    }

    std::unique_ptr<Schema::ICheckpointIndex> CheckpointIndex::CreateICheckpointIndex() const
    {
        if (m_version == Schema::Version{ 1, 0 } ||
            m_version.MajorVersion == 1 ||
            m_version.IsLatest())
        {
            return std::make_unique<Schema::Checkpoint_V1_0::CheckpointIndexInterface>();
        }

        THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
    }

    CheckpointIndex::CheckpointIndex(const std::string& target, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& indexFile) :
        SQLiteStorageBase(target, disposition, std::move(indexFile))
    {
        AICLI_LOG(Repo, Info, << "Opened Checkpoint Index with version [" << m_version << "], last write [" << GetLastWriteTime() << "]");
        m_interface = CreateICheckpointIndex();
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_CANNOT_WRITE_TO_UPLEVEL_INDEX, disposition == SQLiteStorageBase::OpenDisposition::ReadWrite && m_version != m_interface->GetVersion());
    }

    CheckpointIndex::CheckpointIndex(const std::string& target, Schema::Version version) : SQLiteStorageBase(target, version)
    {
        m_interface = CreateICheckpointIndex();
        m_version = m_interface->GetVersion();
    }
}