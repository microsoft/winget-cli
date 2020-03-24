// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct HashCommand final : public Command
    {
        HashCommand() : Command("hash") {}

        virtual std::vector<Argument> GetArguments() const override;

        virtual std::string ShortDescription() const override;
        virtual std::vector<std::string> GetLongDescription() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
