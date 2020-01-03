// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct DescribeCommand final : public Command
    {
        DescribeCommand() : Command("describe") {}

        virtual std::vector<Argument> GetArguments() const override;

        virtual std::string ShortDescription() const override;
        virtual std::vector<std::string> GetLongDescription() const override;
    };
}
