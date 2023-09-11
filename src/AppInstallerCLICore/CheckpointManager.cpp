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
        // This checkpoint name is reserved for the starting checkpoint which captures the automatic metadata.
        constexpr std::string_view s_AutomaticCheckpoint = "automatic"sv;
        constexpr std::string_view s_CheckpointsFileName = "checkpoints.db"sv;
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

        auto indexPath = checkpointsDirectory / s_CheckpointsFileName;
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
        CheckpointRecord::IdType startCheckpointId = m_checkpointRecord->AddCheckpoint(s_AutomaticCheckpoint);
        Checkpoint<AutomaticCheckpointData> checkpoint{ m_checkpointRecord, startCheckpointId };
        return checkpoint;
    }

    Checkpoint<AutomaticCheckpointData> CheckpointManager::GetAutomaticCheckpoint()
    {
        std::optional<CheckpointRecord::IdType> startCheckpointId = m_checkpointRecord->GetCheckpointIdByName(s_AutomaticCheckpoint);

        if (!startCheckpointId.has_value())
        {
            THROW_HR(E_UNEXPECTED);
        }

        return Checkpoint<AutomaticCheckpointData>{ std::move(m_checkpointRecord), startCheckpointId.value() };
    }

    Checkpoint<CLI::Execution::Data> CheckpointManager::CreateCheckpoint(std::string_view checkpointName)
    {
        CheckpointRecord::IdType startCheckpointId = m_checkpointRecord->AddCheckpoint(checkpointName);
        Checkpoint<CLI::Execution::Data> checkpoint{ m_checkpointRecord, startCheckpointId };
        return checkpoint;
    }

    std::vector<Checkpoint<CLI::Execution::Data>> CheckpointManager::GetCheckpoints()
    {
        std::vector<Checkpoint<CLI::Execution::Data>> checkpoints;
        for (const auto& checkpointName : m_checkpointRecord->GetCheckpoints())
        {
            if (checkpointName == s_AutomaticCheckpoint)
            {
                continue;
            }

            CheckpointRecord::IdType checkpointId = m_checkpointRecord->GetCheckpointIdByName(checkpointName).value();
            checkpoints.emplace_back(Checkpoint<CLI::Execution::Data>{ std::move(m_checkpointRecord), checkpointId });
        }

        return checkpoints;
    }

    void CheckpointManager::CleanUpRecord()
    {
        if (m_checkpointRecord)
        {
            m_checkpointRecord.reset();
        }

        if (m_resumeId != GUID_NULL)
        {
            const auto& checkpointRecordPath = GetCheckpointRecordPath(m_resumeId);

            if (std::filesystem::exists(checkpointRecordPath))
            {
                std::error_code error;
                if (std::filesystem::remove(checkpointRecordPath, error))
                {
                    AICLI_LOG(CLI, Info, << "Checkpoint record deleted: " << checkpointRecordPath);
                }

                const auto& checkpointRecordParentDirectory = checkpointRecordPath.parent_path();
                if (std::filesystem::is_empty(checkpointRecordParentDirectory) && std::filesystem::remove(checkpointRecordParentDirectory, error))
                {
                    AICLI_LOG(CLI, Info, << "Checkpoint record directory deleted: " << checkpointRecordParentDirectory);
                }
            }
        }
    }
}
