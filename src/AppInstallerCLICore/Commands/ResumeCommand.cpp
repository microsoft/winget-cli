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

        CheckpointManager checkpointManager = CheckpointManager::Instance();
        checkpointManager.InitializeFromGuid(checkpointId);

        if (AppInstaller::Runtime::GetClientVersion().get() != checkpointManager.GetClientVersion())
        {
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_CLIENTVERSION_MISMATCH);
        }

        std::string commandName = checkpointManager.GetCommandName();
        std::unique_ptr<Command> commandToResume;

        // Use the root command to obtain all of the available commands.
        std::unique_ptr<Command> rootCommand = std::make_unique<RootCommand>();
        auto commands = rootCommand->GetCommands();

        for (auto& command : commands)
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

        // Create a new context and load from checkpoint
        auto checkpointContext = checkpointManager.CreateContextFromCheckpointIndex();
        checkpointContext->SetExecutingCommand(commandToResume.get());
        checkpointManager.LoadCheckpoint(*checkpointContext, Execution::CheckpointFlags::ArgumentsProcessed);
        commandToResume->Execute(*checkpointContext);
    }
}
