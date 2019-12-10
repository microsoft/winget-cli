// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct DescribeCommand final : public Command
    {
        DescribeCommand() : Command(L"describe") {}

        virtual std::vector<Argument> GetArguments() const override;

        virtual std::wstring ShortDescription() const override;
        virtual std::vector<std::wstring> GetLongDescription() const override;
    };
}
