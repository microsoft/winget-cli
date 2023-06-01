// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "StubCommand.h"
#include "Workflows/StubFlow.h"
#include "Workflows/MSStoreInstallerHandler.h"


namespace AppInstaller::CLI
{
    using namespace AppInstaller::CLI::Workflow;

    // TODO: create
    Utility::LocIndView s_StubCommand_HelpLink = "https://aka.ms/winget-command-stub"_liv;

    // StubCommand
    std::vector<std::unique_ptr<Command>> StubCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
            std::make_unique<StubEnableCommand>(FullName()),
            std::make_unique<StubDisableCommand>(FullName()),
            });
    }

    Resource::LocString StubCommand::ShortDescription() const
    {
        // TODO
        return { Resource::String::Done };
    }

    Resource::LocString StubCommand::LongDescription() const
    {
        // TODO
        return { Resource::String::Done };
    }

    Utility::LocIndView StubCommand::HelpLink() const
    {
        return s_StubCommand_HelpLink;
    }

    void StubCommand::ExecuteInternal(Execution::Context& context) const
    {
        OutputHelp(context.Reporter);
    }

    // StubEnableCommand
    Resource::LocString StubEnableCommand::ShortDescription() const
    {
        return { Resource::String::Done };
    }

    Resource::LocString StubEnableCommand::LongDescription() const
    {
        return { Resource::String::PinAddCommandLongDescription };
    }

    Utility::LocIndView StubEnableCommand::HelpLink() const
    {
        return s_StubCommand_HelpLink;
    }

    void StubEnableCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::VerifyStubSupport <<
            Workflow::AppInstallerStubPreferred <<
            Workflow::AppInstallerUpdate;
    }

    // StubDisableCommand
    Resource::LocString StubDisableCommand::ShortDescription() const
    {
        return { Resource::String::Done };
    }

    Resource::LocString StubDisableCommand::LongDescription() const
    {
        return { Resource::String::PinAddCommandLongDescription };
    }

    Utility::LocIndView StubDisableCommand::HelpLink() const
    {
        return s_StubCommand_HelpLink;
    }

    void StubDisableCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::VerifyStubSupport <<
            Workflow::AppInstallerFullPreferred <<
            Workflow::AppInstallerUpdate;
    }
}
