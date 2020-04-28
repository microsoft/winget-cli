// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct ValidateCommand final : public Command
    {
        ValidateCommand(std::string_view parent) : Command("validate", parent) {}

        virtual std::vector<Argument> GetArguments() const override;

        virtual std::string ShortDescription() const override;
        virtual std::string GetLongDescription() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
