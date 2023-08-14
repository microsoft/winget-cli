// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CheckpointManager.h"
#include "Microsoft/CheckpointIndex.h"
#include "Microsoft/SQLiteStorageBase.h"
#include <Argument.h>
#include <Command.h>

namespace AppInstaller::CLI::Checkpoint
{
    CheckpointManager::CheckpointManager(GUID id)
    {
        m_checkpointId = id;
        AICLI_LOG(CLI, Info, << "Opening checkpoint index with id: " << m_checkpointId);
        auto openDisposition = AppInstaller::Repository::Microsoft::SQLiteStorageBase::OpenDisposition::ReadWrite;
        auto checkpointIndex = AppInstaller::Repository::Microsoft::CheckpointIndex::OpenOrCreateDefault(m_checkpointId, openDisposition);
        if (!checkpointIndex)
        {
            AICLI_LOG(CLI, Error, << "Unable to open checkpoint index.");
            // TODO: Handle failure to open index gracefully.
        }

        m_checkpointIndex = std::move(checkpointIndex);
    }

    CheckpointManager::CheckpointManager(std::string_view commandName, std::string_view commandArguments, std::string_view clientVersion)
    {
        std::ignore = CoCreateGuid(&m_checkpointId);

        AICLI_LOG(CLI, Info, << "Creating checkpoint index with id: " << m_checkpointId);
        auto openDisposition = AppInstaller::Repository::Microsoft::SQLiteStorageBase::OpenDisposition::ReadWrite;
        auto checkpointIndex = AppInstaller::Repository::Microsoft::CheckpointIndex::OpenOrCreateDefault(m_checkpointId, openDisposition);
        if (!checkpointIndex)
        {
            AICLI_LOG(CLI, Error, << "Unable to open checkpoint index.");
            // TODO: Handle failure to open index gracefully.
        }

        m_checkpointIndex = std::move(checkpointIndex);
        m_checkpointIndex->SetCommandName(commandName);
        m_checkpointIndex->SetCommandArguments(commandArguments);
        m_checkpointIndex->SetClientVersion(clientVersion);
    }

    void CheckpointManager::LoadContextData(std::string_view checkpointName, AppInstaller::Manifest::ManifestInstaller& installer)
    {
        int contextData = static_cast<int>(Execution::Data::Installer);
        const auto& map = m_checkpointIndex->GetContextData(checkpointName, contextData);
        installer.Url = map.at("url");
        installer.Arch = Utility::ConvertToArchitectureEnum(map.at("arch"));
    }

    void CheckpointManager::RecordContextData(std::string_view checkpointName, const AppInstaller::Manifest::ManifestInstaller& installer)
    {
        int contextData = static_cast<int>(Execution::Data::Installer);
        m_checkpointIndex->AddContextData(checkpointName, contextData, "url"sv, installer.Url);
        m_checkpointIndex->AddContextData(checkpointName, contextData, "arch"sv, ToString(installer.Arch));
        m_checkpointIndex->AddContextData(checkpointName, contextData, "scope"sv, ScopeToString(installer.Scope));
        m_checkpointIndex->AddContextData(checkpointName, contextData, "installerType"sv, InstallerTypeToString(installer.BaseInstallerType));
        m_checkpointIndex->AddContextData(checkpointName, contextData, "sha256"sv, Utility::SHA256::ConvertToString(installer.Sha256));
        m_checkpointIndex->AddContextData(checkpointName, contextData, "locale"sv, installer.Locale);
    }

    std::string CheckpointManager::GetClientVersion()
    {
        return m_checkpointIndex->GetClientVersion();
    }

    std::string CheckpointManager::GetCommandName()
    {
        return m_checkpointIndex->GetCommandName();
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
        return m_checkpointIndex->GetCommandArguments();
    }
}
