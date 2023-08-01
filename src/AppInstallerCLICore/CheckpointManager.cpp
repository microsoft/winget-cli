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
    bool CheckpointManager::CleanUpIndex()
    {
        // Check if index is empty, if so, remove file and return status.
        if (m_checkpointIndex->IsEmpty())
        {
            // Remove file here if it is empty.
        }

        return true;
    }

    CheckpointManager::~CheckpointManager()
    {
        CleanUpIndex();
    }

    void CheckpointManager::Initialize()
    {
        // Initialize should not be called more than once.
        if (m_checkpointId != GUID_NULL)
        {
            return;
        }

        std::ignore = CoCreateGuid(&m_checkpointId);
        AICLI_LOG(CLI, Info, << "Creating checkpoint index with the corresponding guid: " << m_checkpointId);

        //auto openDisposition = m_readOnly ? SQLiteStorageBase::OpenDisposition::Read : SQLiteStorageBase::OpenDisposition::ReadWrite;
        auto openDisposition = AppInstaller::Repository::Microsoft::SQLiteStorageBase::OpenDisposition::ReadWrite;
        auto checkpointIndex = AppInstaller::Repository::Microsoft::CheckpointIndex::OpenOrCreateDefault(m_checkpointId, openDisposition);
        if (!checkpointIndex)
        {
            AICLI_LOG(CLI, Error, << "Unable to open checkpoint index.");
        }

        m_checkpointIndex = std::move(checkpointIndex);
        m_checkpointIndex->SetClientVersion(AppInstaller::Runtime::GetClientVersion());
    }

    void CheckpointManager::InitializeFromGuid(GUID checkpointId)
    {
        // Initialize should not be called more than once.
        if (m_checkpointId != GUID_NULL)
        {
            return;
        }
        // THROW_HR_IF(E_UNEXPECTED, m_checkpointId != GUID_NULL);

        m_checkpointId = checkpointId;
        auto openDisposition = AppInstaller::Repository::Microsoft::SQLiteStorageBase::OpenDisposition::ReadWrite;
        auto checkpointIndex = AppInstaller::Repository::Microsoft::CheckpointIndex::OpenOrCreateDefault(m_checkpointId, openDisposition);
        if (!checkpointIndex)
        {
            AICLI_LOG(CLI, Error, << "Unable to open checkpoint index.");
            THROW_HR_MSG(HRESULT_FROM_WIN32(ERROR_NOT_FOUND), "The saved state could not be found.");
        }

        m_checkpointIndex = std::move(checkpointIndex);
    }

    void CheckpointManager::AddContext(int contextId)
    {
        m_checkpointIndex->AddContext(contextId);
    }

    void CheckpointManager::RemoveContext(int contextId)
    {
        m_checkpointIndex->RemoveContext(contextId);
    }

    std::unique_ptr<Execution::Context> CheckpointManager::CreateContextFromCheckpointIndex()
    {
        auto checkpointContext = std::make_unique<Execution::Context>(std::cout, std::cin);
        auto previousthreadGlobals = checkpointContext->SetForCurrentThread();
        checkpointContext->EnableSignalTerminationHandler();
        return checkpointContext;
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

    void CheckpointManager::Checkpoint(Execution::Context& context, Execution::CheckpointFlag checkpointFlag)
    {
        Execution::CheckpointFlag contextCheckpointFlag = context.GetCurrentCheckpoint();

        // If the context is ahead of the current checkpoint, we have previously executed this state, load checkpoint state from index.
        // If the context is behind the current checkpoint, we have not yet processed this state, save checkpoint state to index.
        // If the checkpoints are equal, do nothing.
        if (contextCheckpointFlag > checkpointFlag)
        {
            LoadCheckpoint(context, checkpointFlag);
        }
        else if (contextCheckpointFlag < checkpointFlag)
        {
            SaveCheckpoint(context, checkpointFlag);
        }
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
}
