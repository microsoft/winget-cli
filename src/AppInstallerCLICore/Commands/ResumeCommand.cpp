// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ResumeCommand.h"
#include "RootCommand.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;
    using namespace Execution;

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
        // Convert guid argument and load checkpoint file.
        const auto& resumeGuid = Utility::ConvertToUTF16(context.Args.GetArg(Execution::Args::Type::ResumeGuid));

        GUID checkpointId;
        THROW_IF_FAILED(CLSIDFromString(resumeGuid.c_str(), &checkpointId));
        context.LoadCheckpointIndex(checkpointId);

        std::string_view commandName = "install"sv;
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
            }
        }

        context.SetExecutingCommand(commandToResume.get());
        commandToResume->Execute(context);
    }
}
