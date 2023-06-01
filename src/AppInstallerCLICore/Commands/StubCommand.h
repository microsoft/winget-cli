// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    // Command to enable and disable the stub package for AppInstaller.
    // The stub package doesn't contain support for winget configuration.
    struct StubCommand final : public Command
    {
        StubCommand(std::string_view parent) : Command("stub", {}, parent, Visibility::Hidden) {}

        std::vector<std::unique_ptr<Command>> GetCommands() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };

    // Command that sets AppInstaller as the stub package.
    struct StubEnableCommand final : public Command
    {
        StubEnableCommand(std::string_view parent) : Command("enable", parent) {}

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };

    // Command that sets AppInstaller as the full package.
    struct StubDisableCommand final : public Command
    {
        StubDisableCommand(std::string_view parent) : Command("disable", parent) {}

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
