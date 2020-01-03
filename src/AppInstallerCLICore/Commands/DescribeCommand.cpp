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
            Argument{ "application", LOCME("The name of the application to install"), ArgumentType::Positional, true },
            Argument{ "manifest", LOCME("If specified, output the full manifest file"), ArgumentType::Flag },
        };
    }

    std::string DescribeCommand::ShortDescription() const
    {
        return LOCME("Describes the given application");
    }

    std::vector<std::string> DescribeCommand::GetLongDescription() const
    {
        return {
            LOCME("Describes the given application"),
        };
    }
}
