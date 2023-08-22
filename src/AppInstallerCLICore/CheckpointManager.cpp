// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CheckpointManager.h"
#include "Microsoft/CheckpointRecord.h"

namespace AppInstaller::CLI::Checkpoint
{
    using namespace AppInstaller::Repository::Microsoft;

    void CheckpointManager::CreateRecord()
    {
        std::ignore = CoCreateGuid(&m_checkpointId);
        AICLI_LOG(CLI, Info, << "Creating checkpoint index with id: " << m_checkpointId);
        const auto& indexPath = CheckpointRecord::GetCheckpointRecordPath(m_checkpointId);
        m_checkpointRecord = std::make_shared<CheckpointRecord>(CheckpointRecord::CreateNew(indexPath.u8string()));
    }

    void CheckpointManager::LoadRecord(GUID id)
    {
        m_checkpointId = id;
        AICLI_LOG(CLI, Info, << "Opening checkpoint index with id: " << m_checkpointId);
        const auto& indexPath = CheckpointRecord::GetCheckpointRecordPath(m_checkpointId);
        m_checkpointRecord = std::make_shared<CheckpointRecord>(CheckpointRecord::Open(indexPath.u8string()));
    }

    bool CheckpointManager::Exists(std::string_view checkpointName)
    {
        return m_checkpointRecord->CheckpointExists(checkpointName);
    }

    std::string CheckpointManager::GetClientVersion()
    {
        return m_checkpointRecord->GetMetadata(CheckpointMetadata::ClientVersion);
    }

    std::string CheckpointManager::GetCommandName()
    {
        return m_checkpointRecord->GetMetadata(CheckpointMetadata::CommandName);
    }

    std::string CheckpointManager::GetLastCheckpoint()
    {
        return m_checkpointRecord->GetLastCheckpoint();
    }

    std::vector<int> CheckpointManager::GetAvailableContextData(std::string_view checkpointName)
    {
        return m_checkpointRecord->GetAvailableData(checkpointName);
    }

    void CheckpointManager::SetClientVersion(std::string_view value)
    {
        m_checkpointRecord->SetMetadata(CheckpointMetadata::ClientVersion, value);
    }

    void CheckpointManager::SetCommandName(std::string_view value)
    {
        m_checkpointRecord->SetMetadata(CheckpointMetadata::CommandName, value);
    }

    void CheckpointManager::AddContextData(std::string_view checkpointName, int contextData, std::string_view name, std::string_view value, int index)
    {
        if (!Exists(checkpointName))
        {
            m_checkpointRecord->AddCheckpoint(checkpointName);
        }

        m_checkpointRecord->AddContextData(checkpointName, contextData, name, value, index);
    }

    std::vector<std::string> CheckpointManager::GetContextData(std::string_view checkpointName, int contextData)
    {
        return m_checkpointRecord->GetContextData(checkpointName, contextData);
    }

    std::vector<std::string> CheckpointManager::GetContextDataByName(std::string_view checkpointName, int contextData, std::string_view name)
    {
        return m_checkpointRecord->GetContextDataByName(checkpointName, contextData, name);
    }

    void CheckpointManager::DeleteRecord()
    {
        if (m_checkpointRecord)
        {
            m_checkpointRecord.reset();
        }

        if (m_checkpointId != GUID_NULL)
        {
            const auto& checkpointRecordPath = CheckpointRecord::GetCheckpointRecordPath(m_checkpointId);

            if (std::filesystem::exists(checkpointRecordPath))
            {
                std::error_code error;
                if (std::filesystem::remove(checkpointRecordPath, error))
                {
                    AICLI_LOG(CLI, Info, << "Checkpoint index deleted: " << checkpointRecordPath);
                }
            }
        }
    }
}
