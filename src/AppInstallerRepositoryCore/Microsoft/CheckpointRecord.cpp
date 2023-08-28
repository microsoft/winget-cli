// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CheckpointRecord.h"
#include "SQLiteStorageBase.h"
#include "Schema/Checkpoint_1_0/CheckpointRecordInterface.h"

namespace AppInstaller::Repository::Microsoft
{
    namespace
    {
        constexpr std::string_view s_checkpoints_filename = "checkpoints.db"sv;

        constexpr std::string_view s_Checkpoints = "Checkpoints"sv;
        constexpr std::string_view s_ClientVersion = "ClientVersion"sv;
        constexpr std::string_view s_CommandName = "CommandName"sv;

        std::string_view GetCheckpointMetadataString(CheckpointMetadata checkpointMetadata)
        {
            switch (checkpointMetadata)
            {
            case CheckpointMetadata::ClientVersion:
                return s_ClientVersion;
            case CheckpointMetadata::CommandName:
                return s_CommandName;
            default:
                return "unknown"sv;
            }
        }
    }

    CheckpointRecord CheckpointRecord::CreateNew(const std::string& filePath, Schema::Version version)
    {
        AICLI_LOG(Repo, Info, << "Creating new Checkpoint Index with version [" << version << "] at '" << filePath << "'");
        CheckpointRecord result{ filePath, version };

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(result.m_dbconn, "CheckpointRecord_createnew");

        // Use calculated version, as incoming version could be 'latest'
        result.m_version.SetSchemaVersion(result.m_dbconn);

        result.m_interface->CreateTables(result.m_dbconn);

        result.SetLastWriteTime();

        savepoint.Commit();

        return result;
    }

    std::filesystem::path CheckpointRecord::GetCheckpointRecordPath(GUID guid)
    {
        wchar_t checkpointGuid[256];
        THROW_HR_IF(E_UNEXPECTED, !StringFromGUID2(guid, checkpointGuid, ARRAYSIZE(checkpointGuid)));

        const auto checkpointsDirectory = Runtime::GetPathTo(Runtime::PathName::CheckpointsLocation) / checkpointGuid;

        if (!std::filesystem::exists(checkpointsDirectory))
        {
            std::filesystem::create_directories(checkpointsDirectory);
            AICLI_LOG(Repo, Info, << "Creating checkpoint index directory: " << checkpointsDirectory);
        }
        else
        {
            THROW_HR_IF(ERROR_CANNOT_MAKE, !std::filesystem::is_directory(checkpointsDirectory));
        }

        auto indexPath = checkpointsDirectory / s_checkpoints_filename;
        return indexPath;
    }

    bool CheckpointRecord::IsEmpty()
    {
        return m_interface->IsEmpty(m_dbconn);
    }



    std::vector<int> CheckpointRecord::GetAvailableData(std::string_view name)
    {
        return m_interface->GetAvailableContextData(m_dbconn, name);
    }

    std::string CheckpointRecord::GetMetadata(CheckpointMetadata checkpointMetadata)
    {
        const auto& metadataName = GetCheckpointMetadataString(checkpointMetadata);
        return m_interface->GetMetadata(m_dbconn, metadataName);
    }

    CheckpointRecord::IdType CheckpointRecord::SetMetadata(CheckpointMetadata checkpointMetadata, std::string_view value)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Setting client version [" << value << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "checkpointindex_setmetadata");

        const auto& metadataName = GetCheckpointMetadataString(checkpointMetadata);
        IdType result = m_interface->SetMetadata(m_dbconn, metadataName, value);

        SetLastWriteTime();
        savepoint.Commit();
        return result;
    }

    CheckpointRecord::IdType CheckpointRecord::AddCheckpoint(std::string_view checkpointName)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Adding checkpoint [" << checkpointName << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "checkpointrecord_addcheckpoint");

        IdType result = m_interface->AddCheckpoint(m_dbconn, checkpointName);

        SetLastWriteTime();
        savepoint.Commit();
        return result;
    }

    bool CheckpointRecord::CheckpointExists(std::string_view checkpointName)
    {
        return m_interface->CheckpointExists(m_dbconn, checkpointName);
    }

    CheckpointRecord::IdType CheckpointRecord::AddContextData(std::string_view checkpointName, int contextData, std::string_view name, std::string_view value, int index)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Setting context data [" << contextData << "] for [" << name << "] with value [" << value << "] value");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "checkpointindex_addcontextdata");
        SQLite::rowid_t rowId = m_interface->AddContextData(m_dbconn, checkpointName, contextData, name, value, index);
        savepoint.Commit();
        return rowId;
    }

    std::string CheckpointRecord::GetLastCheckpoint()
    {
        return m_interface->GetLastCheckpoint(m_dbconn);
    }

    std::vector<std::string> CheckpointRecord::GetContextData(std::string_view checkpointName, int contextData)
    {
        return m_interface->GetContextData(m_dbconn, checkpointName, contextData);
    }

    std::vector<std::string> CheckpointRecord::GetContextDataByName(std::string_view checkpointName, int contextData, std::string_view name)
    {
        return m_interface->GetContextDataByName(m_dbconn, checkpointName, contextData, name);
    }

    void CheckpointRecord::RemoveContextData(std::string_view checkpointName, int contextData)
    {
        std::lock_guard<std::mutex> lockInterface{ *m_interfaceLock };
        AICLI_LOG(Repo, Verbose, << "Removing context data [" << contextData << "]");

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(m_dbconn, "checkpointindex_addcontextdata");
        m_interface->RemoveContextData(m_dbconn, checkpointName, contextData);
        savepoint.Commit();
    }

    std::unique_ptr<Schema::ICheckpointRecord> CheckpointRecord::CreateICheckpointRecord() const
    {
        if (m_version == Schema::Version{ 1, 0 } ||
            m_version.MajorVersion == 1 ||
            m_version.IsLatest())
        {
            return std::make_unique<Schema::Checkpoint_V1_0::CheckpointRecordInterface>();
        }

        THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
    }

    CheckpointRecord::CheckpointRecord(const std::string& target, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& indexFile) :
        SQLiteStorageBase(target, disposition, std::move(indexFile))
    {
        AICLI_LOG(Repo, Info, << "Opened Checkpoint Index with version [" << m_version << "], last write [" << GetLastWriteTime() << "]");
        m_interface = CreateICheckpointRecord();
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_CANNOT_WRITE_TO_UPLEVEL_INDEX, disposition == SQLiteStorageBase::OpenDisposition::ReadWrite && m_version != m_interface->GetVersion());
    }

    CheckpointRecord::CheckpointRecord(const std::string& target, Schema::Version version) : SQLiteStorageBase(target, version)
    {
        m_interface = CreateICheckpointRecord();
        m_version = m_interface->GetVersion();
    }
}