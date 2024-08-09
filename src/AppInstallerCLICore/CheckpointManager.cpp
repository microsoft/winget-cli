// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CheckpointManager.h"
#include "Command.h"
#include "ExecutionContextData.h"
#include <AppInstallerRuntime.h>

using namespace AppInstaller::CLI;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::SQLite;

namespace AppInstaller::Checkpoints
{

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
                THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE), !std::filesystem::is_directory(checkpointsDirectory));
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

        automaticCheckpoint.Set(AutomaticCheckpointData::ResumeCount, {}, std::to_string(0));
    }

    void LoadCommandArgsFromAutomaticCheckpoint(CLI::Execution::Context& context, Checkpoint<AutomaticCheckpointData>& automaticCheckpoint)
    {
        for (const auto& fieldName : automaticCheckpoint.GetFieldNames(AutomaticCheckpointData::Arguments))
        {
            // Command arguments are represented as integer strings in the checkpoint record.
            Execution::Args::Type type = static_cast<Execution::Args::Type>(std::stoi(fieldName));
            auto argumentType = Argument::ForType(type).Type();
            if (argumentType == ArgumentType::Flag)
            {
                context.Args.AddArg(type);
            }
            else
            {
                const auto& values = automaticCheckpoint.GetMany(AutomaticCheckpointData::Arguments, fieldName);
                for (const auto& value : values)
                {
                    context.Args.AddArg(type, value);
                }
            }
        }
    }

    std::optional<Checkpoint<AutomaticCheckpointData>> CheckpointManager::GetAutomaticCheckpoint()
    {
        const auto& checkpointIds = m_checkpointDatabase->GetCheckpointIds();
        if (checkpointIds.empty())
        {
            return {};
        }

        CheckpointDatabase::IdType automaticCheckpointId = checkpointIds.back();
        return Checkpoint<AutomaticCheckpointData>{ m_checkpointDatabase, automaticCheckpointId };
    }

    Checkpoint<CLI::Execution::Data> CheckpointManager::CreateCheckpoint(std::string_view checkpointName)
    {
        CheckpointDatabase::IdType checkpointId = m_checkpointDatabase->AddCheckpoint(checkpointName);
        Checkpoint<CLI::Execution::Data> checkpoint{ m_checkpointDatabase, checkpointId };
        return checkpoint;
    }

    std::vector<Checkpoint<CLI::Execution::Data>> CheckpointManager::GetCheckpoints()
    {
        auto checkpointIds = m_checkpointDatabase->GetCheckpointIds();
        if (checkpointIds.empty())
        {
            return {};
        }

        // Remove the last checkpoint (automatic)
        checkpointIds.pop_back();

        std::vector<Checkpoint<CLI::Execution::Data>> checkpoints;
        for (const auto& checkpointId : checkpointIds)
        {
            checkpoints.emplace_back(Checkpoint<CLI::Execution::Data>{ m_checkpointDatabase, checkpointId });
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
                const auto& checkpointDatabaseParentDirectory = checkpointDatabasePath.parent_path();
                AICLI_LOG(CLI, Info, << "Deleting Checkpoint database directory: " << checkpointDatabaseParentDirectory);
                std::filesystem::remove_all(checkpointDatabaseParentDirectory);
            }
        }
    }
}
