// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CheckpointManager.h"
#include "Microsoft/CheckpointIndex.h"
#include "Microsoft/SQLiteStorageBase.h"

namespace AppInstaller::CLI::Checkpoint
{
    CheckpointManager::CheckpointManager(GUID checkpointId) : m_checkpointId(checkpointId)
    {
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
        
        // Change to loading in context from checkpoint.
        PopulateContextArgsFromCheckpointIndex(*(checkpointContext.get()));
        return checkpointContext;
    }

    void CheckpointManager::PopulateContextArgsFromCheckpointIndex(Execution::Context& context)
    {
        std::vector<std::pair<int, std::string>> args = m_checkpointIndex->GetArguments();

        for (auto arg : args)
        {
            context.Args.AddArg(static_cast<Execution::Args::Type>(arg.first), arg.second);
        }
    }

    void CheckpointManager::InitializeCheckpoint(std::string_view clientVersion, std::string_view commandName)
    {
        std::ignore = CoCreateGuid(&m_checkpointId);
        AICLI_LOG(CLI, Info, << "Creating checkpoint index with the corresponding guid: " << m_checkpointId);

        //auto openDisposition = m_readOnly ? SQLiteStorageBase::OpenDisposition::Read : SQLiteStorageBase::OpenDisposition::ReadWrite;
        auto openDisposition = AppInstaller::Repository::Microsoft::SQLiteStorageBase::OpenDisposition::ReadWrite;
        auto checkpointIndex = AppInstaller::Repository::Microsoft::CheckpointIndex::OpenOrCreateDefault(m_checkpointId, openDisposition);
        if (!checkpointIndex)
        {
            AICLI_LOG(CLI, Error, << "Unable to open savepoint index.");
        }
        m_checkpointIndex = std::move(checkpointIndex);

        m_checkpointIndex->SetClientVersion(clientVersion);
        m_checkpointIndex->SetCommandName(commandName);
    }

    void CheckpointManager::SaveCheckpoint(Execution::Context& context, Execution::CheckpointFlags flag)
    {
        if (flag == Execution::CheckpointFlags::ArgumentsProcessed)
        {
            // Get all args and write them to the index.
            const std::vector<Execution::Args::Type>& arguments = context.Args.GetTypes();

            for (auto argument : arguments)
            {
                const auto& argumentValue = context.Args.GetArg(argument);

                // Write current arg to table.
                m_checkpointIndex->AddCommandArgument(static_cast<int>(argument), argumentValue);
            }
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
