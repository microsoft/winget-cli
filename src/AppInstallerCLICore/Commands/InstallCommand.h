// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct InstallCommand final : public Command
    {
        InstallCommand() : Command("install") {}

        std::vector<Argument> GetArguments() const override;

        std::string ShortDescription() const override;
        std::vector<std::string> GetLongDescription() const override;

    protected:
        void ExecuteInternal(Invocation& inv, std::ostream& out, std::istream& in) const override;
        void ValidateArguments(Invocation& inv) const override;
    };
}
