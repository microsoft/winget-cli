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
            Argument{ L"application", LOCME(L"The name of the application to install"), ArgumentType::Positional, true },
        };
    }

    std::wstring InstallCommand::ShortDescription() const
    {
        return LOCME(L"Installs the given application");
    }

    std::vector<std::wstring> InstallCommand::GetLongDescription() const
    {
        return {
            LOCME(L"Installs the given application"),
        };
    }
}
