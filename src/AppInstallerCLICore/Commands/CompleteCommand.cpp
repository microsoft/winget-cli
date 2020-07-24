// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CompleteCommand.h"
#include "RootCommand.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;

    using namespace Execution;

    std::vector<Argument> CompleteCommand::GetArguments() const
    {
        return {
            Argument{ "word", Argument::NoAlias, Args::Type::Word, Resource::String::WordArgumentDescription, ArgumentType::Standard, Argument::Visibility::Example, true },
            Argument{ "commandline", Argument::NoAlias, Args::Type::CommandLine, Resource::String::CommandLineArgumentDescription, ArgumentType::Standard, Argument::Visibility::Example, true },
            Argument{ "position", Argument::NoAlias, Args::Type::Position, Resource::String::PositionArgumentDescription, ArgumentType::Standard, Argument::Visibility::Example, true },
        };
    }

    Resource::LocString CompleteCommand::ShortDescription() const
    {
        return { Resource::String::CompleteCommandShortDescription };
    }

    Resource::LocString CompleteCommand::LongDescription() const
    {
        return { Resource::String::CompleteCommandLongDescription };
    }

    std::string CompleteCommand::HelpLink() const
    {
        // TODO: Define me and point to the right location
        return "https://aka.ms/winget-command-complete";
    }

    void CompleteCommand::ExecuteInternal(Execution::Context& context) const
    {
        CompletionData data{
            context.Args.GetArg(Args::Type::Word),
            context.Args.GetArg(Args::Type::CommandLine),
            context.Args.GetArg(Args::Type::Position) };

        std::unique_ptr<Command> command = data.FindCommand(std::make_unique<RootCommand>());
        AICLI_LOG(CLI, Info, << "Complete handing off to command " << command->FullName());

        command->Complete(context, data);
    }
}
