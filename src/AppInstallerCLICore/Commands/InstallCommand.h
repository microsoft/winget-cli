// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct InstallCommand final : public Command
    {
        InstallCommand(std::string_view parent) : Command("install", parent) {}

        std::vector<Argument> GetArguments() const override;

        std::string ShortDescription() const override;
        std::string GetLongDescription() const override;

    protected:
        void ValidateArgumentsInternal(Execution::Args& execArgs) const override;
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
