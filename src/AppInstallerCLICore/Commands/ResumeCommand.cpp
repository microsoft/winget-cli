// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerRuntime.h"
#include "Resources.h"
#include "ResumeCommand.h"
#include "RootCommand.h"
#include "CheckpointManager.h"
#include "Microsoft/CheckpointRecord.h"
#include "Workflows/ResumeFlow.h"

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;
    using namespace Execution;
    using namespace Checkpoint;

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
        std::string resumeGuidString{ context.Args.GetArg(Execution::Args::Type::ResumeId) };
        GUID checkpointId = Utility::ConvertToGuid(resumeGuidString);

        if (!std::filesystem::exists(AppInstaller::Repository::Microsoft::CheckpointRecord::GetCheckpointRecordPath(checkpointId)))
        {
            context.Reporter.Error() << Resource::String::ResumeIdNotFoundError(Utility::LocIndView{ resumeGuidString }) << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_RESUME_ID_NOT_FOUND);
        }

        Execution::Context resumeContext = context.CreateEmptyContext();

        CheckpointManager checkpointManager = resumeContext.CheckpointManager;
        checkpointManager.LoadExistingRecord(checkpointId);


        const auto& checkpointClientVersion = checkpointManager.GetClientVersion();
        if (checkpointClientVersion != AppInstaller::Runtime::GetClientVersion().get())
        {
            context.Reporter.Error() << Resource::String::ClientVersionMismatchError(Utility::LocIndView{ checkpointClientVersion }) << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_CLIENT_VERSION_MISMATCH);
        }

        std::string commandName = checkpointManager.GetCommandName();
        std::unique_ptr<Command> commandToResume;

        // Find the command using the root command.
        AICLI_LOG(CLI, Info, << "Resuming command: " << commandName);
        for (auto& command : std::make_unique<RootCommand>()->GetCommands())
        {
            if (Utility::CaseInsensitiveEquals(commandName, command->Name()))
            {
                commandToResume = std::move(command);
                break;
            }
        }

        THROW_HR_IF_MSG(E_UNEXPECTED, !commandToResume, "Command to resume not found.");

        resumeContext.SetExecutingCommand(commandToResume.get());
        resumeContext.SetFlags(Execution::ContextFlag::Resume);

        // Load arguments here:

        auto previousThreadGlobals = resumeContext.SetForCurrentThread();
        resumeContext.EnableSignalTerminationHandler();
        commandToResume->Resume(resumeContext);
        context.SetTerminationHR(resumeContext.GetTerminationHR());
    }

    void ResumeCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        std::string resumeGuidString{ execArgs.GetArg(Execution::Args::Type::ResumeId) };
        if (!Utility::IsValidGuidString(resumeGuidString))
        {
            throw CommandException(Resource::String::InvalidResumeIdError(Utility::LocIndView{ resumeGuidString }));
        }
    }
}
