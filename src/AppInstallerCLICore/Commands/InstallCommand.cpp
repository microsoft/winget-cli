// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Common.h"
#include "InstallCommand.h"
#include "Localization.h"
#include "Manifest\Manifest.h"
#include "Workflows\InstallFlow.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Workflow;

namespace AppInstaller::CLI
{
    std::vector<Argument> InstallCommand::GetArguments() const
    {
        return {
            Argument{ ARG_QUERY, LOCME("The name of the application to install"), ArgumentType::Positional, false },
            Argument{ ARG_MANIFEST, LOCME("The path to the manifest of the application to install"), ArgumentType::Standard, false },
            Argument{ ARG_INTERACTIVE, LOCME("The application installation is interactive. User input is needed."), ArgumentType::Flag, false },
            Argument{ ARG_SILENT, LOCME("The application installation is silent."), ArgumentType::Flag, false },
            Argument{ ARG_LANGUAGE, LOCME("Preferred language if application installation supports multiple languages."), ArgumentType::Standard, false },
            Argument{ ARG_LOG, LOCME("Preferred log location if application installation supports custom log path."), ArgumentType::Standard, false },
            Argument{ ARG_OVERRIDE, LOCME("Override switches to be passed on to application installer."), ArgumentType::Standard, false },
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

    void InstallCommand::ExecuteInternal(Invocation& inv, std::ostream& out, std::istream& in) const
    {
        InstallFlow appInstall(inv, out, in);

        appInstall.Execute();
    }

    void InstallCommand::ValidateArguments(Invocation& inv) const
    {
        Command::ValidateArguments(inv);

        if (!inv.Contains(ARG_QUERY) && !inv.Contains(ARG_MANIFEST))
        {
            throw CommandException(LOCME("Required argument not provided"), ARG_QUERY);
        }

        if (inv.Contains(ARG_SILENT) && inv.Contains(ARG_INTERACTIVE))
        {
            throw CommandException(LOCME("More than one install behavior argument provided"), ARG_QUERY);
        }
    }
}
