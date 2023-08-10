// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerRuntime.h"
#include "CheckpointManager.h"
#include "Microsoft/CheckpointIndex.h"
#include "Microsoft/SQLiteStorageBase.h"
#include <Argument.h>
#include <Command.h>

namespace AppInstaller::CLI::Checkpoint
{
    CheckpointManager::CheckpointManager(GUID id)
    {
        if (m_checkpointId == GUID_NULL)
        {
            std::ignore = CoCreateGuid(&m_checkpointId);
        }

        AICLI_LOG(CLI, Info, << "Checkpoint manager id: " << m_checkpointId);

        auto openDisposition = AppInstaller::Repository::Microsoft::SQLiteStorageBase::OpenDisposition::ReadWrite;
        auto checkpointIndex = AppInstaller::Repository::Microsoft::CheckpointIndex::OpenOrCreateDefault(m_checkpointId, openDisposition);
        if (!checkpointIndex)
        {
            AICLI_LOG(CLI, Error, << "Unable to open checkpoint index.");
            // TODO: Handle failure to open index gracefully.
        }

        m_checkpointIndex = std::move(checkpointIndex);
        m_checkpointIndex->SetClientVersion(AppInstaller::Runtime::GetClientVersion());
    }

    template<>
    void CheckpointManager::RecordContextData(std::string_view checkpointName, Manifest::ManifestInstaller installer)
    {
        // Capture all relevant data from the installer.
        m_checkpointIndex->SetCommandName(0, installer.PackageFamilyName);
    };

    void CheckpointManager::RecordMetadata(
        std::string_view checkpointName,
        std::string_view commandName,
        std::string_view commandLineString,
        std::string clientVersion)
    {
        // Capture these arguments from the checkpoint metadata.
    }

    std::string CheckpointManager::GetClientVersion()
    {
        return m_checkpointIndex->GetClientVersion();
    }

    std::string CheckpointManager::GetCommandName(int contextId)
    {
        return m_checkpointIndex->GetCommandName(contextId);
    }

    void CheckpointManager::CleanUpIndex()
    {
        bool isIndexEmpty = m_checkpointIndex->IsEmpty();

        m_checkpointIndex.reset();

        if (isIndexEmpty)
        {
            const auto& checkpointIndexPath = AppInstaller::Repository::Microsoft::CheckpointIndex::GetCheckpointIndexPath(m_checkpointId);
            if (std::filesystem::remove(checkpointIndexPath))
            {
                AICLI_LOG(CLI, Info, << "Checkpoint index deleted: " << checkpointIndexPath);
            }
        }
    }

    CheckpointManager::~CheckpointManager()
    {
        CleanUpIndex();
    }

    std::string CheckpointManager::GetArguments()
    {
        // Return the actual command line arguments.
        m_checkpointIndex->GetCommandName(0);
    }
}
