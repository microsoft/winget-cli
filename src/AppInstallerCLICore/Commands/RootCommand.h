// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct RootCommand final : public Command
    {
        RootCommand() : Command("root") {}

        virtual std::vector<std::unique_ptr<Command>> GetCommands() const override;

        virtual std::vector<std::string> GetLongDescription() const override;

    protected:
        virtual void ExecuteInternal(Invocation& inv, std::ostream& out) const;
    };
}
