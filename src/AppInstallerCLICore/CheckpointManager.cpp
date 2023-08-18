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
        THROW_HR_IF(E_UNEXPECTED, IsLoaded());
        std::ignore = CoCreateGuid(&m_checkpointId);
        AICLI_LOG(CLI, Info, << "Creating checkpoint index with id: " << m_checkpointId);
        m_checkpointRecord = CheckpointRecord::CreateDefault(m_checkpointId);
    }

    CheckpointManager::CheckpointManager()
    {
        m_checkpointId = {};
    }

    void CheckpointManager::LoadExistingRecord(GUID id)
    {
        m_checkpointId = id;
        AICLI_LOG(CLI, Info, << "Opening checkpoint index with id: " << m_checkpointId);
        m_checkpointRecord = CheckpointRecord::OpenDefault(m_checkpointId);
    }

    std::string CheckpointManager::GetClientVersion()
    {
        THROW_HR_IF(E_UNEXPECTED, !IsLoaded());
        return m_checkpointRecord->GetMetadata(CheckpointMetadata::ClientVersion);
    }

    std::string CheckpointManager::GetCommandName()
    {
        THROW_HR_IF(E_UNEXPECTED, !IsLoaded());
        return m_checkpointRecord->GetMetadata(CheckpointMetadata::CommandName);
    }

    std::string CheckpointManager::GetArguments()
    {
        return {};
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
        const auto& checkpointId = m_checkpointRecord->GetCheckpointId(checkpointName);

        if (checkpointId)
        {
            m_checkpointRecord->AddContextData(checkpointId.value(), contextData, name, value, index);
        }
    }

    void CheckpointManager::CleanUpIndex()
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
