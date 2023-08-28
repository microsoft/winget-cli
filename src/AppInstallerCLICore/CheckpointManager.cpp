// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CheckpointManager.h"
#include "Microsoft/Schema/Checkpoint_1_0/CheckpointRecordInterface.h"
#include <AppInstallerRuntime.h>

namespace AppInstaller::Checkpoints
{
    using namespace AppInstaller::Repository::Microsoft;
    using namespace AppInstaller::Repository::SQLite;

    namespace
    {
        constexpr std::string_view s_checkpoints_filename = "checkpoints.db"sv;

        constexpr std::string_view s_Checkpoints = "Checkpoints"sv;
        constexpr std::string_view s_ClientVersion = "ClientVersion"sv;
        constexpr std::string_view s_CommandName = "CommandName"sv;

        std::string_view GetCheckpointMetadataString(AutomaticCheckpointData checkpointMetadata)
        {
            switch (checkpointMetadata)
            {
            case AutomaticCheckpointData::ClientVersion:
                return s_ClientVersion;
            case AutomaticCheckpointData::CommandName:
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
