// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CheckpointManager.h"
#include <AppInstallerRuntime.h>
#include "ExecutionContextData.h"

namespace AppInstaller::Checkpoints
{
    using namespace AppInstaller::Repository::Microsoft;
    using namespace AppInstaller::Repository::SQLite;

    namespace
    {
        constexpr std::string_view s_checkpoints_filename = "checkpoints.db"sv;

        // This checkpoint name is reserved for the starting checkpoint which captures the automatic metadata.
        constexpr std::string_view s_StartCheckpoint = "start"sv;
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

    std::filesystem::path CheckpointManager::GetCheckpointRecordPath(GUID guid)
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

    CheckpointManager::CheckpointManager()
    {
        std::ignore = CoCreateGuid(&m_resumeId);
        const auto& checkpointRecordPath = GetCheckpointRecordPath(m_resumeId);
        m_checkpointRecord = std::make_shared<CheckpointRecord>(CheckpointRecord::CreateNew(checkpointRecordPath.u8string()));
    }

    CheckpointManager::CheckpointManager(GUID resumeId)
    {
        m_resumeId = resumeId;
        const auto& checkpointRecordPath = GetCheckpointRecordPath(m_resumeId);
        m_checkpointRecord = std::make_shared<CheckpointRecord>(CheckpointRecord::Open(checkpointRecordPath.u8string()));
    }

    Checkpoint<AutomaticCheckpointData> CheckpointManager::CreateAutomaticCheckpoint()
    {
        CheckpointRecord::IdType startCheckpointId = m_checkpointRecord->AddCheckpoint(s_StartCheckpoint);
        Checkpoint<AutomaticCheckpointData> checkpoint{ m_checkpointRecord, startCheckpointId };
        return checkpoint;
    }

    Checkpoint<CLI::Execution::Data> CheckpointManager::CreateCheckpoint(std::string_view checkpointName)
    {
        CheckpointRecord::IdType startCheckpointId = m_checkpointRecord->AddCheckpoint(checkpointName);
        Checkpoint<CLI::Execution::Data> checkpoint{ m_checkpointRecord, startCheckpointId };
        return checkpoint;
    }

    Checkpoint<AutomaticCheckpointData> CheckpointManager::GetAutomaticCheckpoint()
    {
        std::optional<CheckpointRecord::IdType> startCheckpointId = m_checkpointRecord->GetCheckpointIdByName(s_StartCheckpoint);

        if (!startCheckpointId.has_value())
        {
            THROW_HR(E_UNEXPECTED);
        }

        return Checkpoint<AutomaticCheckpointData>{ std::move(m_checkpointRecord), startCheckpointId.value() };
    }

    std::vector<Checkpoint<CLI::Execution::Data>> CheckpointManager::GetCheckpoints()
    {
        std::vector<Checkpoint<CLI::Execution::Data>> checkpoints;
        for (const auto& checkpoint : m_checkpointRecord->GetCheckpoints())
        {
            // Exclude starting checkpoint from these context data related checkpoints.
            if (checkpoint == s_StartCheckpoint)
            {
                continue;
            }

            CheckpointRecord::IdType checkpointId = m_checkpointRecord->GetCheckpointIdByName(checkpoint).value();
            checkpoints.emplace_back(Checkpoint<CLI::Execution::Data>{ std::move(m_checkpointRecord), checkpointId });
        }

        return checkpoints;
    }
}
