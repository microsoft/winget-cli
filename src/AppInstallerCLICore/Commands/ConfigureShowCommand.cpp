// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigureShowCommand.h"

namespace AppInstaller::CLI
{
    std::vector<Argument> ConfigureShowCommand::GetArguments() const
    {
        return {};
    }

    Resource::LocString ConfigureShowCommand::ShortDescription() const
    {
        return { Resource::String::ConfigureShowCommandShortDescription };
    }

    Resource::LocString ConfigureShowCommand::LongDescription() const
    {
        return { Resource::String::ConfigureShowCommandLongDescription };
    }

    Utility::LocIndView ConfigureShowCommand::HelpLink() const
    {
        // TODO: Make this exist
        return "https://aka.ms/winget-command-configure#show-command"_liv;
    }

    void ConfigureShowCommand::ExecuteInternal(Execution::Context& context) const
    {
        THROW_HR(E_NOTIMPL);
    }
}
