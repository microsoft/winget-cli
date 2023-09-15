// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    // This command makes it easier to get information on winget errors.
    struct ErrorCommand final : public Command
    {
        ErrorCommand(std::string_view parent) : Command("error", { "err" }, parent, Visibility::Hidden) {}

        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
