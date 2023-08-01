// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerRuntime.h"
#include "CheckpointManager.h"
#include "ResumeCommand.h"
#include "RootCommand.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;
    using namespace Execution;
    using namespace Checkpoint;

    std::vector<Argument> ResumeCommand::GetArguments() const
    {
        return {
            Argument::ForType(Execution::Args::Type::ResumeGuid),
        };
    }

    Resource::LocString ResumeCommand::ShortDescription() const
    {
        return { Resource::String::ResumeCommandShortDescription };
    }

    Resource::LocString ResumeCommand::LongDescription() const
    {
        return { Resource::String::ResumeCommandLongDescription };
    }

    Utility::LocIndView ResumeCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-resume"_liv;
    }

    void ResumeCommand::ExecuteInternal(Execution::Context& context) const
    {
        GUID checkpointId = Utility::ConvertToGuid(std::string{ context.Args.GetArg(Execution::Args::Type::ResumeGuid) });

        CheckpointManager::Instance().InitializeFromGuid(checkpointId);

        if (AppInstaller::Runtime::GetClientVersion().get() != CheckpointManager::Instance().GetClientVersion())
        {
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_CLIENTVERSION_MISMATCH);
        }

        // Get the root context id from the index.
        int rootContextId = CheckpointManager::Instance().GetFirstContextId();

        // Determine the command to execute.
        std::string commandName = CheckpointManager::Instance().GetCommandName(rootContextId);
        std::unique_ptr<Command> commandToResume;

        for (auto& command : std::make_unique<RootCommand>()->GetCommands())
        {
            if (
                Utility::CaseInsensitiveEquals(commandName, command->Name()) ||
                Utility::CaseInsensitiveContains(command->Aliases(), commandName)
                )
            {
                AICLI_LOG(CLI, Info, << "Resuming command: " << commandName);
                commandToResume = std::move(command);
                break;
            }
        }

        // Create a new context, set the executing command and current checkpoint.
        auto resumeContext = context.CreateEmptyContext(rootContextId);
        resumeContext->SetExecutingCommand(commandToResume.get());

        // Set the current checkpoint of the root context.
        resumeContext->SetCurrentCheckpoint(CheckpointManager::Instance().GetLastCheckpoint(rootContextId));

        // Load the arguments from the checkpoint index prior to executing the command.
        CheckpointManager::Instance().Checkpoint(*resumeContext, Execution::CheckpointFlag::CommandArguments);
        commandToResume->Execute(*resumeContext);
    }
}
