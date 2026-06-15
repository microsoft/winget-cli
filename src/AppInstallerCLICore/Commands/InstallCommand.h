// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct InstallCommand final : public Command
    {
        InstallCommand(std::string_view parent) : Command("install", { "add" }, parent) {}

        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        void Complete(Execution::Context& context, Execution::Args::Type valueType) const override;

        void Resume(Execution::Context& context) const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ValidateArgumentsInternal(Execution::Args& execArgs) const override;
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
