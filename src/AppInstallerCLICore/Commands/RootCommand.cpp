// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RootCommand.h"

#include "InstallCommand.h"
#include "ShowCommand.h"
#include "SourceCommand.h"
#include "SearchCommand.h"
#include "HashCommand.h"
#include "ValidateCommand.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    std::vector<std::unique_ptr<Command>> RootCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
            std::make_unique<InstallCommand>(FullName()),
            std::make_unique<ShowCommand>(FullName()),
            std::make_unique<SourceCommand>(FullName()),
            std::make_unique<SearchCommand>(FullName()),
            std::make_unique<HashCommand>(FullName()),
            std::make_unique<ValidateCommand>(FullName()),
        });
    }

    std::vector<Argument> RootCommand::GetArguments() const
    {
        return
        {
            Argument{ "version", 'v', Execution::Args::Type::ListVersions, Resource::String::ToolVersionArgumentDescription, ArgumentType::Flag, Visibility::Help },
            Argument{ "info", APPINSTALLER_CLI_ARGUMENT_NO_SHORT_VER, Execution::Args::Type::Info, Resource::String::ToolInfoArgumentDescription, ArgumentType::Flag, Visibility::Help },
        };
    }

    Resource::LocString RootCommand::LongDescription() const
    {
        return { Resource::String::ToolDescription };
    }

    std::string RootCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-help";
    }

    void RootCommand::ExecuteInternal(Execution::Context& context) const
    {
        if (context.Args.Contains(Execution::Args::Type::Info))
        {
            OutputIntroHeader(context.Reporter);

            context.Reporter.Info() << std::endl <<
                "Links:" << std::endl <<
                "  Privacy Statement: https://aka.ms/winget-privacy" << std::endl <<
                "  License agreement: https://aka.ms/winget-license" << std::endl <<
                "  3rd Party Notices: https://aka.ms/winget-3rdPartyNotice" << std::endl <<
                "  Homepage:          https://aka.ms/winget" << std::endl;
        }
        else if (context.Args.Contains(Execution::Args::Type::ListVersions))
        {
            context.Reporter.Info() << 'v' << Runtime::GetClientVersion() << ' ' << Resource::String::PreviewVersion << std::endl;
        }
        else
        {
            OutputHelp(context.Reporter);
        }
    }
}
