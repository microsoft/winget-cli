// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallCommand.h"
#include "Localization.h"

namespace AppInstaller::CLI
{
    std::vector<Argument> InstallCommand::GetArguments() const
    {
        return {
            Argument{ "application", LOCME("The name of the application to install"), ArgumentType::Positional, true },
        };
    }

    std::string InstallCommand::ShortDescription() const
    {
        return LOCME("Installs the given application");
    }

    std::vector<std::string> InstallCommand::GetLongDescription() const
    {
        return {
            LOCME("Installs the given application"),
        };
    }
}
