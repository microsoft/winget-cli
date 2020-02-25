// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct ShowCommand final : public Command
    {
        ShowCommand() : Command("show") {}

        virtual std::vector<Argument> GetArguments() const override;

        virtual std::string ShortDescription() const override;
        virtual std::vector<std::string> GetLongDescription() const override;

    protected:
        void ExecuteInternal(Invocation& inv, std::ostream& out, std::istream& in) const override;
    };
}
