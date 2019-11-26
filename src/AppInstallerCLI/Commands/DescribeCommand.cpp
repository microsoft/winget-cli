// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DescribeCommand.h"
#include "Localization.h"

namespace AppInstaller::CLI
{
    std::vector<Argument> DescribeCommand::GetArguments() const
    {
        return {
            Argument{ L"application", LOCME(L"The name of the application to install"), ArgumentType::Positional, true },
            Argument{ L"manifest", LOCME(L"If specified, output the full manifest file"), ArgumentType::Flag },
        };
    }

    std::wstring DescribeCommand::ShortDescription() const
    {
        return LOCME(L"Describes the given application");
    }

    std::vector<std::wstring> DescribeCommand::GetLongDescription() const
    {
        return {
            LOCME(L"Describes the given application"),
        };
    }
}
