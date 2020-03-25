// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RootCommand.h"
#include "Localization.h"

#include "InstallCommand.h"
#include "ShowCommand.h"
#include "SourceCommand.h"
#include "SearchCommand.h"
#include "HashCommand.h"

namespace AppInstaller::CLI
{
    std::vector<std::unique_ptr<Command>> RootCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
            std::make_unique<InstallCommand>(Name()),
            std::make_unique<ShowCommand>(Name()),
            std::make_unique<SourceCommand>(Name()),
            std::make_unique<SearchCommand>(Name()),
            std::make_unique<HashCommand>(Name()),
        });
    }

    std::vector<Argument> RootCommand::GetArguments() const
    {
        return
        {
            Argument{ "version", 'v', Execution::Args::Type::ListVersions, LOCME("Display the version of the tool"), ArgumentType::Flag, Visibility::Help },
        };
    }

    std::string RootCommand::GetLongDescription() const
    {
        return LOCME("AppInstaller command line utility enables installing applications from the command line.");
    }

    void RootCommand::ExecuteInternal(Execution::Context& context) const
    {
        if (context.Args.Contains(Execution::Args::Type::ListVersions))
        {
            context.Reporter.Info() << Runtime::GetClientVersion() << std::endl;
        }
        else
        {
            OutputHelp(context.Reporter);
        }
    }
}
