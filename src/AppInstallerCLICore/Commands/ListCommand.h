// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    // Command to get the set of installed packages on the system.
    struct ListCommand final : public Command
    {
        ListCommand(std::string_view parent) : Command("list", parent) {}

        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        void Complete(Execution::Context& context, Execution::Args::Type valueType) const override;

        std::string HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
