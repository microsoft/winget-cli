// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RootCommand.h"
#include "Localization.h"

#include "InstallCommand.h"
#include "DescribeCommand.h"

namespace AppInstaller::CLI
{
    std::vector<std::unique_ptr<Command>> RootCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
            std::make_unique<InstallCommand>(),
            std::make_unique<DescribeCommand>(),
        });
    }

    std::vector<std::string> RootCommand::GetLongDescription() const
    {
        return {
            LOCME("AppInstaller command line utility enables installing applications from the"),
            LOCME("command line."),
        };
    }

    void RootCommand::ExecuteInternal(Invocation&, std::ostream& out) const
    {
        OutputHelp(out);
    }
}
