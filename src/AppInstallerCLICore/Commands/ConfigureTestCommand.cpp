// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigureTestCommand.h"

namespace AppInstaller::CLI
{
    std::vector<Argument> ConfigureTestCommand::GetArguments() const
    {
        return {};
    }

    Resource::LocString ConfigureTestCommand::ShortDescription() const
    {
        return { Resource::String::ConfigureTestCommandShortDescription };
    }

    Resource::LocString ConfigureTestCommand::LongDescription() const
    {
        return { Resource::String::ConfigureTestCommandLongDescription };
    }

    Utility::LocIndView ConfigureTestCommand::HelpLink() const
    {
        // TODO: Make this exist
        return "https://aka.ms/winget-command-configure#test"_liv;
    }

    void ConfigureTestCommand::ExecuteInternal(Execution::Context& context) const
    {
        Command::ExecuteInternal(context);
    }
}
