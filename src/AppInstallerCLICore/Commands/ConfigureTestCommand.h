// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct ConfigureTestCommand final : public Command
    {
        ConfigureTestCommand(std::string_view parent) : Command("test", parent) {}

        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
        void ValidateArgumentsInternal(Execution::Args& execArgs) const override;
        void Complete(Execution::Context& context, Execution::Args::Type argType) const override;
    };
}
