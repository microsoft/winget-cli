// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CheckpointManager.h"
#include "Command.h"
#include "ExecutionContextData.h"
#include <AppInstallerRuntime.h>

namespace AppInstaller::Checkpoints
{
    using namespace AppInstaller::Repository::Microsoft;
    using namespace AppInstaller::Repository::SQLite;
    using namespace AppInstaller::CLI;

    // This checkpoint name is reserved for the starting checkpoint which captures the automatic metadata.
    constexpr std::string_view s_AutomaticCheckpoint = "automatic"sv;
    constexpr std::string_view s_CheckpointsFileName = "checkpoints.db"sv;

    std::filesystem::path CheckpointManager::GetCheckpointDatabasePath(const std::string_view& resumeId, bool createCheckpointDirectory)
    {

        const auto checkpointsDirectory = Runtime::GetPathTo(Runtime::PathName::CheckpointsLocation) / resumeId;

        if (createCheckpointDirectory)
        {
            if (!std::filesystem::exists(checkpointsDirectory))
            {
                AICLI_LOG(Repo, Info, << "Creating checkpoint database directory: " << checkpointsDirectory);
                std::filesystem::create_directories(checkpointsDirectory);
            }
            else
            {
                THROW_HR_IF(ERROR_CANNOT_MAKE, !std::filesystem::is_directory(checkpointsDirectory));
            }
        }

        auto recordPath = checkpointsDirectory / s_CheckpointsFileName;
        return recordPath;
    }

    CheckpointManager::CheckpointManager()
    {
        GUID resumeId;
        std::ignore = CoCreateGuid(&resumeId);
        m_resumeId = Utility::ConvertGuidToString(resumeId);
        const auto& checkpointDatabasePath = GetCheckpointDatabasePath(m_resumeId, true);
        m_checkpointDatabase = CheckpointDatabase::CreateNew(checkpointDatabasePath.u8string());
    }

    CheckpointManager::CheckpointManager(const std::string& resumeId)
    {
        m_resumeId = resumeId;
        const auto& checkpointDatabasePath = GetCheckpointDatabasePath(m_resumeId);
        m_checkpointDatabase = CheckpointDatabase::Open(checkpointDatabasePath.u8string());
    }

    void CheckpointManager::CreateAutomaticCheckpoint(CLI::Execution::Context& context)
    {
        CheckpointDatabase::IdType startCheckpointId = m_checkpointDatabase->AddCheckpoint(s_AutomaticCheckpoint);
        Checkpoint<AutomaticCheckpointData> automaticCheckpoint{ m_checkpointDatabase, startCheckpointId };

        automaticCheckpoint.Set(AutomaticCheckpointData::ClientVersion, {}, AppInstaller::Runtime::GetClientVersion());

        const auto& executingCommand = context.GetExecutingCommand();
        if (executingCommand != nullptr)
        {
            automaticCheckpoint.Set(AutomaticCheckpointData::Command, {}, std::string{ executingCommand->FullName() });
        }

        const auto& argTypes = context.Args.GetTypes();
        for (auto type : argTypes)
        {
            const auto& argument = std::to_string(static_cast<int>(type));
            auto argumentType = Argument::ForType(type).Type();

            if (argumentType == ArgumentType::Flag)
            {
                automaticCheckpoint.Set(AutomaticCheckpointData::Arguments, argument, {});
            }
            else
            {
                const auto& values = *context.Args.GetArgs(type);
                automaticCheckpoint.SetMany(AutomaticCheckpointData::Arguments, argument, values);
            }
        }
    }

    Checkpoint<AutomaticCheckpointData> CheckpointManager::GetAutomaticCheckpoint()
    {
        const auto& checkpointIds = m_checkpointDatabase->GetCheckpointIds();
        if (checkpointIds.empty())
        {
            THROW_HR(E_UNEXPECTED);
        }

        const auto& automaticCheckpointId = checkpointIds.back();
        return Checkpoint<AutomaticCheckpointData>{ std::move(m_checkpointDatabase), automaticCheckpointId };
    }

    Checkpoint<CLI::Execution::Data> CheckpointManager::CreateCheckpoint(std::string_view checkpointName)
    {
        CheckpointDatabase::IdType startCheckpointId = m_checkpointDatabase->AddCheckpoint(checkpointName);
        Checkpoint<CLI::Execution::Data> checkpoint{ m_checkpointDatabase, startCheckpointId };
        return checkpoint;
    }

    std::vector<Checkpoint<CLI::Execution::Data>> CheckpointManager::GetCheckpoints()
    {
        auto checkpointIds = m_checkpointDatabase->GetCheckpointIds();

        // Remove the last checkpoint (automatic)
        checkpointIds.pop_back();

        std::vector<Checkpoint<CLI::Execution::Data>> checkpoints;
        for (const auto& checkpointId : checkpointIds)
        {
            checkpoints.emplace_back(Checkpoint<CLI::Execution::Data>{ std::move(m_checkpointDatabase), checkpointId });
        }

        return checkpoints;
    }

    void CheckpointManager::CleanUpDatabase()
    {
        if (m_checkpointDatabase)
        {
            m_checkpointDatabase.reset();
        }

        if (!m_resumeId.empty())
        {
            const auto& checkpointDatabasePath = GetCheckpointDatabasePath(m_resumeId);

            if (std::filesystem::exists(checkpointDatabasePath))
            {
                std::error_code error;
                if (std::filesystem::remove(checkpointDatabasePath, error))
                {
                    AICLI_LOG(CLI, Info, << "Checkpoint database deleted: " << checkpointDatabasePath);
                }

                const auto& checkpointDatabaseParentDirectory = checkpointDatabasePath.parent_path();
                if (std::filesystem::remove(checkpointDatabaseParentDirectory, error))
                {
                    AICLI_LOG(CLI, Info, << "Checkpoint database directory deleted: " << checkpointDatabaseParentDirectory);
                }
            }
        }
    }
}
