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
    void CheckpointManager::Initialize()
    {
        // Initialize should not be called more than once.
        THROW_HR_IF(E_UNEXPECTED, m_checkpointId != GUID_NULL);

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
        //m_checkpointIndex->SetCommandName(commandName);
    }

    void CheckpointManager::InitializeFromGuid(GUID checkpointId)
    {
        // Initialize should not be called more than once.
        THROW_HR_IF(E_UNEXPECTED, m_checkpointId != GUID_NULL);

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

    std::unique_ptr<Execution::Context> CheckpointManager::CreateContextFromCheckpointIndex()
    {
        auto checkpointContext = std::make_unique<Execution::Context>(std::cout, std::cin);
        auto previousthreadGlobals = checkpointContext->SetForCurrentThread();

        checkpointContext->EnableSignalTerminationHandler();
        
        PopulateContextArgsFromIndex(*checkpointContext);
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
                        if (m_checkpointIndex->GetBoolArgumentByContextId(contextId, argument.Name()))
                        {
                            context.Args.AddArg(executionArgsType);
                        }
                    }
                    else
                    {
                        context.Args.AddArg(executionArgsType, m_checkpointIndex->GetStringArgumentByContextId(contextId, argument.Name()));
                    }
                }
            }
        }
    }

    void CheckpointManager::RecordContextArgsToIndex(Execution::Context& context)
    {
        int contextId = context.GetContextId();

        // Figure out if there is a better place to add the context when it is first initialized.
        m_checkpointIndex->AddContextToArgumentTable(contextId);

        const auto& executingCommand = context.GetExecutingCommand();
        if (executingCommand != nullptr)
        {
            const auto& commandArguments = executingCommand->GetArguments();
            for (const auto& argument : commandArguments)
            {
                Execution::Args::Type type = argument.ExecArgType();
                if (context.Args.Contains(type))
                {
                    if (argument.Type() == ArgumentType::Flag)
                    {
                        m_checkpointIndex->UpdateArgumentByContextId(contextId, argument.Name(), true);
                    }
                    else
                    {
                        const auto& argumentValue = context.Args.GetArg(type);
                        m_checkpointIndex->UpdateArgumentByContextId(contextId, argument.Name(), argumentValue);
                    }
                }
            }
        }
    }

    void CheckpointManager::SaveCheckpoint(Execution::Context& context, Execution::CheckpointFlags flag)
    {
        switch (flag)
        {
        case Execution::CheckpointFlags::ArgumentsProcessed:
            RecordContextArgsToIndex(context);
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    void CheckpointManager::LoadCheckpoint(Execution::Context& context, Execution::CheckpointFlags flag)
    {
        switch (flag)
        {
        case Execution::CheckpointFlags::ArgumentsProcessed:
            PopulateContextArgsFromIndex(context);
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    std::string CheckpointManager::GetClientVersion()
    {
        // Need to throw exception if checkpoint index is null.
        return m_checkpointIndex->GetClientVersion();
    }

    std::string CheckpointManager::GetCommandName()
    {
        return m_checkpointIndex->GetCommandName();
    }
}
