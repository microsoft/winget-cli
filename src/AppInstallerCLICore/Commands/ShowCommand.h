// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct ShowCommand final : public Command
    {
        ShowCommand(std::string_view parent) : Command("show", parent) {}

        virtual std::vector<Argument> GetArguments() const override;

        virtual std::string ShortDescription() const override;
        virtual std::vector<std::string> GetLongDescription() const override;

    protected:
        void ExecuteInternal(AppInstaller::CLI::Execution::Context& context) const override;
    };
}
