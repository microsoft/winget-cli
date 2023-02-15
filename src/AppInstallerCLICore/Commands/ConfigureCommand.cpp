// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigureCommand.h"
#include "ConfigureShowCommand.h"
#include "ConfigureTestCommand.h"
#include "ConfigureValidateCommand.h"

namespace AppInstaller::CLI
{
    std::vector<std::unique_ptr<Command>> ConfigureCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
            std::make_unique<ConfigureShowCommand>(FullName()),
            std::make_unique<ConfigureTestCommand>(FullName()),
            std::make_unique<ConfigureValidateCommand>(FullName()),
        });
    }

    std::vector<Argument> ConfigureCommand::GetArguments() const
    {
        return {};
    }

    Resource::LocString ConfigureCommand::ShortDescription() const
    {
        return { Resource::String::ConfigureCommandShortDescription };
    }

    Resource::LocString ConfigureCommand::LongDescription() const
    {
        return { Resource::String::ConfigureCommandLongDescription };
    }

    Utility::LocIndView ConfigureCommand::HelpLink() const
    {
        // TODO: Make this exist
        return "https://aka.ms/winget-command-configure"_liv;
    }

    void ConfigureCommand::ExecuteInternal(Execution::Context& context) const
    {
        THROW_HR(E_NOTIMPL);
    }
}
