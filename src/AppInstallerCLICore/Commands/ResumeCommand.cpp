// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerRuntime.h"
#include "CheckpointManager.h"
#include "ResumeCommand.h"
#include "Workflows/ResumeFlow.h"
#include "Resources.h"
#include "RootCommand.h"

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
        context <<
            Workflow::EnsureSupportForResume;

        if (context.IsTerminated())
        {
            return;
        }

        int rootContextId = CheckpointManager::Instance().GetFirstContextId();
        auto resumeContextPtr = context.CreateEmptyContext(rootContextId);

        Context& resumeContext = *resumeContextPtr;
        auto previousThreadGlobals = resumeContext.SetForCurrentThread();

        std::string commandName = CheckpointManager::Instance().GetCommandName(rootContextId);
        std::unique_ptr<Command> commandToResume;

        // Find the command using the command name.
        AICLI_LOG(CLI, Info, << "Resuming command: " << commandName);
        for (auto& command : std::make_unique<RootCommand>()->GetCommands())
        {
            if (
                Utility::CaseInsensitiveEquals(commandName, command->Name()) ||
                Utility::CaseInsensitiveContains(command->Aliases(), commandName)
                )
            {
                commandToResume = std::move(command);
                break;
            }
        }

        THROW_HR_IF_MSG(E_UNEXPECTED, !commandToResume, "Command to resume not found.");

        resumeContext.SetExecutingCommand(commandToResume.get());
        resumeContext.SetTargetCheckpoint(CheckpointManager::Instance().GetLastCheckpoint(rootContextId));
        resumeContext.SetFlags(Execution::ContextFlag::Resume);

        CheckpointManager::Instance().Checkpoint(resumeContext, Execution::CheckpointFlag::CommandArguments);
        
        commandToResume->Execute(resumeContext);

        // If the resumeContext is terminated, set the termination HR to report the correct HR from the resume context.
        if (resumeContext.IsTerminated())
        {
            context.SetTerminationHR(resumeContext.GetTerminationHR());
        }
    }
}
