// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerRuntime.h"
#include "Resources.h"
#include "ResumeCommand.h"
#include "RootCommand.h"
#include "CheckpointManager.h"
#include "Workflows/ResumeFlow.h"

using namespace AppInstaller::Checkpoints;

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
        const auto& resumeId = context.Args.GetArg(Execution::Args::Type::ResumeId);

        if (!std::filesystem::exists(Checkpoints::CheckpointManager::GetCheckpointDatabasePath(resumeId)))
        {
            context.Reporter.Error() << Resource::String::ResumeIdNotFoundError(Utility::LocIndView{ resumeId }) << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_RESUME_ID_NOT_FOUND);
        }

        Execution::Context resumeContext = context.CreateEmptyContext();
        std::optional<Checkpoint<AutomaticCheckpointData>> foundAutomaticCheckpoint = resumeContext.LoadCheckpoint(std::string{ resumeId });
        if (!foundAutomaticCheckpoint.has_value())
        {
            context.Reporter.Error() << Resource::String::ResumeStateDataNotFoundError << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INVALID_RESUME_STATE);
        }

        Checkpoint<AutomaticCheckpointData> automaticCheckpoint = foundAutomaticCheckpoint.value();

        const auto& checkpointClientVersion = automaticCheckpoint.Get(AutomaticCheckpointData::ClientVersion, {});
        if (checkpointClientVersion != AppInstaller::Runtime::GetClientVersion().get())
        {
            context.Reporter.Error() << Resource::String::ClientVersionMismatchError(Utility::LocIndView{ checkpointClientVersion }) << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_CLIENT_VERSION_MISMATCH);
        }

        const auto& checkpointCommand = automaticCheckpoint.Get(AutomaticCheckpointData::Command, {});

        AICLI_LOG(CLI, Info, << "Resuming command: " << checkpointCommand);
        std::unique_ptr<Command> commandToResume;
        std::unique_ptr<AppInstaller::CLI::Command> currentCommand = std::make_unique<RootCommand>();

        // TODO: Handle command parsing for multiple commands
        for (const auto& checkpointCommandPart : Utility::Split(checkpointCommand, ' '))
        {
            for (auto& command : currentCommand->GetCommands())
            {
                if (Utility::CaseInsensitiveEquals(checkpointCommandPart, command->FullName()))
                {
                    currentCommand = std::move(command);
                    break;
                }
            }
        }

        commandToResume = std::move(currentCommand);

        THROW_HR_IF_MSG(E_UNEXPECTED, !commandToResume, "Command to resume not found.");

        for (const auto& fieldName : automaticCheckpoint.GetFieldNames(AutomaticCheckpointData::Arguments))
        {
            // Command arguments are represented as integer strings in the checkpoint record.
            Execution::Args::Type type = static_cast<Execution::Args::Type>(std::stoi(fieldName));
            auto argumentType = Argument::ForType(type).Type();
            if (argumentType == ArgumentType::Flag)
            {
                resumeContext.Args.AddArg(type);
            }
            else
            {
                const auto& values = automaticCheckpoint.GetMany(AutomaticCheckpointData::Arguments, fieldName);
                for (const auto& value : values)
                {
                    resumeContext.Args.AddArg(type, value);
                }
            }
        }

        resumeContext.SetExecutingCommand(commandToResume.get());

        // TODO: Ensure telemetry is properly handled for resume context.
        resumeContext.SetFlags(Execution::ContextFlag::Resume);

        auto previousThreadGlobals = resumeContext.SetForCurrentThread();
        resumeContext.EnableSignalTerminationHandler();
        commandToResume->Resume(resumeContext);
        context.SetTerminationHR(resumeContext.GetTerminationHR());
    }
}
