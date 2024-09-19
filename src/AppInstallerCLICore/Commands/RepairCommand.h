// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct RepairCommand final : public Command
    {
        RepairCommand(std::string_view parent) : Command("repair", { "fix" }, parent) {}

        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        void Complete(Execution::Context& context, Execution::Args::Type valueType) const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ValidateArgumentsInternal(Execution::Args& execArgs) const override;
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
