// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CheckpointManager.h"
#include "Microsoft/CheckpointIndex.h"
#include "Microsoft/SQLiteStorageBase.h"
#include <AppInstallerErrors.h>

namespace AppInstaller::CLI::Checkpoint
{
    using namespace AppInstaller::Repository::Microsoft;

    CheckpointManager::CheckpointManager(GUID id)
    {
        m_checkpointId = id;
        AICLI_LOG(CLI, Info, << "Opening checkpoint index with id: " << m_checkpointId);
        auto openDisposition = SQLiteStorageBase::OpenDisposition::ReadWrite;
        auto checkpointIndex = CheckpointIndex::OpenOrCreateDefault(m_checkpointId, openDisposition);
        if (!checkpointIndex)
        {
            AICLI_LOG(CLI, Error, << "Unable to open checkpoint index.");
            THROW_HR_MSG(APPINSTALLER_CLI_ERROR_CANNOT_OPEN_CHECKPOINT_INDEX, "The checkpoint index could not be opened.");
        }

        m_checkpointIndex = std::move(checkpointIndex);
    }

    CheckpointManager::CheckpointManager(std::string_view commandName, std::string_view commandArguments, std::string_view clientVersion)
    {
        std::ignore = CoCreateGuid(&m_checkpointId);

        AICLI_LOG(CLI, Info, << "Creating checkpoint index with id: " << m_checkpointId);
        auto openDisposition = SQLiteStorageBase::OpenDisposition::ReadWrite;
        auto checkpointIndex = CheckpointIndex::OpenOrCreateDefault(m_checkpointId, openDisposition);
        if (!checkpointIndex)
        {
            AICLI_LOG(CLI, Error, << "Unable to open checkpoint index.");
            return;
        }

        m_checkpointIndex = std::move(checkpointIndex);
        m_checkpointIndex->SetCommandName(commandName);
        m_checkpointIndex->SetCommandArguments(commandArguments);
        m_checkpointIndex->SetClientVersion(clientVersion);
    }

    std::string CheckpointManager::GetClientVersion()
    {
        return m_checkpointIndex->GetClientVersion();
    }

    std::string CheckpointManager::GetCommandName()
    {
        return m_checkpointIndex->GetCommandName();
    }

    std::string CheckpointManager::GetArguments()
    {
        return m_checkpointIndex->GetCommandArguments();
    }

    void CheckpointManager::CleanUpIndex()
    {
        if (m_checkpointIndex)
        {
            m_checkpointIndex.reset();
        }

        if (m_checkpointId != GUID_NULL)
        {
            const auto& checkpointIndexPath = CheckpointIndex::GetCheckpointIndexPath(m_checkpointId);

            if (std::filesystem::exists(checkpointIndexPath))
            {
                std::error_code error;
                if (std::filesystem::remove(checkpointIndexPath, error))
                {
                    AICLI_LOG(CLI, Info, << "Checkpoint index deleted: " << checkpointIndexPath);
                }
            }
        }
    }
}
