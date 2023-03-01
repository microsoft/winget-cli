// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigureValidateCommand.h"

namespace AppInstaller::CLI
{
    std::vector<Argument> ConfigureValidateCommand::GetArguments() const
    {
        return {};
    }

    Resource::LocString ConfigureValidateCommand::ShortDescription() const
    {
        return { Resource::String::ConfigureValidateCommandShortDescription };
    }

    Resource::LocString ConfigureValidateCommand::LongDescription() const
    {
        return { Resource::String::ConfigureValidateCommandLongDescription };
    }

    Utility::LocIndView ConfigureValidateCommand::HelpLink() const
    {
        // TODO: Make this exist
        return "https://aka.ms/winget-command-configure#validate"_liv;
    }

    void ConfigureValidateCommand::ExecuteInternal(Execution::Context& context) const
    {
        Command::ExecuteInternal(context);
    }
}
