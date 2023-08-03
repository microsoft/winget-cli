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
    void CheckpointManager::Initialize(GUID checkpointId)
    {
        // Initialize should only be called once.
        THROW_HR_IF(E_UNEXPECTED, m_checkpointId != GUID_NULL);

        if (checkpointId == GUID_NULL)
        {
            std::ignore = CoCreateGuid(&m_checkpointId);
            AICLI_LOG(CLI, Info, << "Creating checkpoint index with guid: " << m_checkpointId);
        }
        else
        {
            m_checkpointId = checkpointId;
            AICLI_LOG(CLI, Info, << "Opening checkpoint index with guid: " << m_checkpointId);
        }

        auto openDisposition = AppInstaller::Repository::Microsoft::SQLiteStorageBase::OpenDisposition::ReadWrite;
        auto checkpointIndex = AppInstaller::Repository::Microsoft::CheckpointIndex::OpenOrCreateDefault(m_checkpointId, openDisposition);
        if (!checkpointIndex)
        {
            AICLI_LOG(CLI, Error, << "Unable to open checkpoint index.");
            THROW_HR_MSG(HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "The saved state could not be found.");
        }

        m_checkpointIndex = std::move(checkpointIndex);
        m_checkpointIndex->SetClientVersion(AppInstaller::Runtime::GetClientVersion());
    }

    void CheckpointManager::Checkpoint(Execution::Context& context, Execution::CheckpointFlag checkpointFlag)
    {
        Execution::CheckpointFlag contextCheckpointFlag = context.GetTargetCheckpoint();

        // If the context target checkpoint is ahead or equal to the current checkpoint, we have previously executed this state, load checkpoint state from index.
        // If the context target checkpoint is behind the current checkpoint, we have not yet processed this state, save checkpoint state to index.
        if (contextCheckpointFlag > checkpointFlag || contextCheckpointFlag == checkpointFlag)
        {
            LoadCheckpoint(context, checkpointFlag);
        }
        else if (contextCheckpointFlag < checkpointFlag)
        {
            SaveCheckpoint(context, checkpointFlag);
        }
    }

    void CheckpointManager::AddContext(int contextId)
    {
        m_checkpointIndex->AddContext(contextId);
    }

    void CheckpointManager::RemoveContext(int contextId)
    {
        m_checkpointIndex->RemoveContext(contextId);
    }

    std::string CheckpointManager::GetClientVersion()
    {
        // Need to throw exception if checkpoint index is null.
        return m_checkpointIndex->GetClientVersion();
    }

    std::string CheckpointManager::GetCommandName(int contextId)
    {
        return m_checkpointIndex->GetCommandName(contextId);
    }

    int CheckpointManager::GetFirstContextId()
    {
        return m_checkpointIndex->GetFirstContextId();
    }

    Execution::CheckpointFlag CheckpointManager::GetLastCheckpoint(int contextId)
    {
        return static_cast<Execution::CheckpointFlag>(m_checkpointIndex->GetLastCheckpoint(contextId));
    }

    bool CheckpointManager::CleanUpIndex()
    {
        if (m_checkpointIndex->IsEmpty())
        {
            m_checkpointIndex.reset();

            const auto& checkpointIndexPath = AppInstaller::Repository::Microsoft::CheckpointIndex::GetCheckpointIndexPath(m_checkpointId);
            if (std::filesystem::remove(checkpointIndexPath))
            {
                AICLI_LOG(CLI, Info, << "Checkpoint index deleted: " << checkpointIndexPath);
            }
        }

        return true;
    }

    CheckpointManager::~CheckpointManager()
    {
        CleanUpIndex();
    }

    void CheckpointManager::SaveCheckpoint(Execution::Context& context, Execution::CheckpointFlag flag)
    {
        switch (flag)
        {
        case Execution::CheckpointFlag::CommandArguments:
            RecordContextArgsToIndex(context);
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }

        m_checkpointIndex->SetLastCheckpoint(context.GetContextId(), static_cast<int>(flag));
        context.SetCurrentCheckpoint(flag);
    }

    void CheckpointManager::LoadCheckpoint(Execution::Context& context, Execution::CheckpointFlag flag)
    {
        switch (flag)
        {
        case Execution::CheckpointFlag::CommandArguments:
            PopulateContextArgsFromIndex(context);
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }

        context.SetCurrentCheckpoint(flag);
    }

    void CheckpointManager::PopulateContextArgsFromIndex(Execution::Context& context)
    {
        int contextId = context.GetContextId();

        const auto& executingCommand = context.GetExecutingCommand();
        if (executingCommand != nullptr)
        {
            const auto& commandArguments = executingCommand->GetArguments();
            for (const auto& argument : commandArguments)
            {
                if (m_checkpointIndex->ContainsArgument(contextId, argument.Name()))
                {
                    Execution::Args::Type executionArgsType = argument.ExecArgType();
                    if (argument.Type() == ArgumentType::Flag)
                    {
                        if (m_checkpointIndex->GetBoolArgument(contextId, argument.Name()))
                        {
                            context.Args.AddArg(executionArgsType);
                        }
                    }
                    else
                    {
                        context.Args.AddArg(executionArgsType, m_checkpointIndex->GetStringArgument(contextId, argument.Name()));
                    }
                }
            }
        }
    }

    void CheckpointManager::RecordContextArgsToIndex(Execution::Context& context)
    {
        int contextId = context.GetContextId();

        const auto& executingCommand = context.GetExecutingCommand();
        if (executingCommand != nullptr)
        {
            m_checkpointIndex->SetCommandName(contextId, executingCommand->Name());

            const auto& commandArguments = executingCommand->GetArguments();
            for (const auto& argument : commandArguments)
            {
                Execution::Args::Type type = argument.ExecArgType();
                if (context.Args.Contains(type))
                {
                    if (argument.Type() == ArgumentType::Flag)
                    {
                        m_checkpointIndex->UpdateArgument(contextId, argument.Name(), true);
                    }
                    else
                    {
                        const auto& argumentValue = context.Args.GetArg(type);
                        m_checkpointIndex->UpdateArgument(contextId, argument.Name(), argumentValue);
                    }
                }
            }
        }
    }
}
