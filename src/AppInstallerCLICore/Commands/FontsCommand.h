// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"
#include <winget/UserSettings.h>

namespace AppInstaller::CLI
{
    struct FontsCommand final : public Command
    {
        FontsCommand(std::string_view parent) : Command("fonts", {} /* aliases */, parent) {}

        std::vector<std::unique_ptr<Command>> GetCommands() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };

    struct FontsListCommand final : public Command
    {
        FontsListCommand(std::string_view parent) : Command("list", parent) {}

        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        void Complete(Execution::Context& context, Execution::Args::Type valueType) const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
