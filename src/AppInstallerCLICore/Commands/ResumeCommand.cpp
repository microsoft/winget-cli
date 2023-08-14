// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerRuntime.h"
#include "Resources.h"
#include "ResumeCommand.h"
#include "RootCommand.h"
#include "Microsoft/CheckpointIndex.h"
#include "Workflows/ResumeFlow.h"

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;
    using namespace Execution;

    std::vector<Argument> ResumeCommand::GetArguments() const
    {
        return {
            Argument::ForType(Execution::Args::Type::ResumeId),
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
        std::string resumeGuidString { context.Args.GetArg(Execution::Args::Type::ResumeId) };
        if (!Utility::IsValidGuidString(resumeGuidString))
        {
            context.Reporter.Error() << Resource::String::InvalidResumeGuidError(Utility::LocIndView{ resumeGuidString }) << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INVALID_RESUME_GUID);
        }

        GUID checkpointId = Utility::ConvertToGuid(resumeGuidString);

        if (!std::filesystem::exists(AppInstaller::Repository::Microsoft::CheckpointIndex::GetCheckpointIndexPath(checkpointId)))
        {
            context.Reporter.Error() << Resource::String::ResumeGuidNotFoundError(Utility::LocIndView{ resumeGuidString }) << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_RESUME_GUID_NOT_FOUND);
        }


        Execution::Context resumeContext{ std::cout, std::cin };
        auto previousThreadGlobals = resumeContext.SetForCurrentThread();
        resumeContext.EnableSignalTerminationHandler();
        resumeContext.InitializeCheckpointManager(checkpointId);

        std::string commandName = resumeContext.GetCheckpointCommand();
        std::unique_ptr<Command> commandToResume;

        // Find the command using the root command.
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
        resumeContext.SetFlags(Execution::ContextFlag::Resume);
        commandToResume->Resume(resumeContext);
    }
}
