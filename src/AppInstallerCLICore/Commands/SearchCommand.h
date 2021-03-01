// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct SearchCommand final : public Command
    {
        SearchCommand(std::string_view parent) : Command("search", parent) {}

        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        void Complete(Execution::Context& context, Execution::Args::Type valueType) const override;

        std::string HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
