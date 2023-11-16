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
    namespace
    {
        std::unique_ptr<Command> FindCommandToResume(const std::string& commandFullName)
        {
            std::unique_ptr<Command> commandToResume = std::make_unique<RootCommand>();

            for (const auto& commandPart : Utility::Split(commandFullName, ':'))
            {
                bool commandFound = false;
                if (Utility::CaseInsensitiveEquals(commandPart, commandToResume->Name()))
                {
                    // Since we always expect to start at the 'root' command, skip and check the next command part. 
                    continue;
                }

                for (auto& command : commandToResume->GetCommands())
                {
                    if (Utility::CaseInsensitiveEquals(commandPart, command->Name()))
                    {
                        commandFound = true;
                        commandToResume = std::move(command);
                        break;
                    }
                }

                if (!commandFound)
                {
                    THROW_HR_MSG(E_UNEXPECTED, "Command to resume not found.");
                }
            }

            return std::move(commandToResume);
        }
    }

    using namespace std::string_view_literals;
    using namespace Execution;

    std::vector<Argument> ResumeCommand::GetArguments() const
    {
        return {
            Argument::ForType(Execution::Args::Type::ResumeId),
            Argument::ForType(Execution::Args::Type::IgnoreResumeLimit),
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

        const auto& resumeCountString = automaticCheckpoint.Get(AutomaticCheckpointData::ResumeCount, {});
        int resumeCount = std::stoi(resumeCountString);
        if (!context.Args.Contains(Execution::Args::Type::IgnoreResumeLimit) && resumeCount >= Settings::User().Get<Settings::Setting::MaxResumes>())
        {
            std::string manualResumeString = "winget resume -g " + std::string{ resumeId } + " --ignore-resume-limit";
            context.Reporter.Error() << Resource::String::ResumeLimitExceeded(Utility::LocIndView{ resumeCountString }) << Utility::LocIndView{ manualResumeString } << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_RESUME_LIMIT_EXCEEDED);
        }
        else
        {
            automaticCheckpoint.Update(AutomaticCheckpointData::ResumeCount, {}, std::to_string(resumeCount + 1));
        }

        const auto& checkpointCommand = automaticCheckpoint.Get(AutomaticCheckpointData::Command, {});
        AICLI_LOG(CLI, Info, << "Resuming command: " << checkpointCommand);
        std::unique_ptr<Command> commandToResume = FindCommandToResume(checkpointCommand);

        LoadCommandArgsFromAutomaticCheckpoint(resumeContext, automaticCheckpoint);

        resumeContext.SetExecutingCommand(commandToResume.get());

        // TODO: Ensure telemetry is properly handled for resume context.
        resumeContext.SetFlags(Execution::ContextFlag::Resume);

        auto previousThreadGlobals = resumeContext.SetForCurrentThread();
        resumeContext.EnableSignalTerminationHandler();
        commandToResume->Resume(resumeContext);
        context.SetTerminationHR(resumeContext.GetTerminationHR());
    }
}
