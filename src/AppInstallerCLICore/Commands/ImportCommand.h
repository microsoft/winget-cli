// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    // Command to install a set of packages from a list.
    struct ImportCommand final : public Command
    {
        ImportCommand(std::string_view parent) : Command("import", parent) {}

        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        std::string HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
