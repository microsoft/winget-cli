// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    // Command to enable tab completion scenarios, including allowing them to
    // be context sensitive in their data output.
    struct CompleteCommand final : public Command
    {
        CompleteCommand(std::string_view parent) : Command("complete", parent, Visibility::Hidden) {}

        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        std::string HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
