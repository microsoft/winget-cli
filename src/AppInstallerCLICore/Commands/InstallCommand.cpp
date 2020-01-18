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
            Argument{ ARG_APPLICATION, LOCME("The name of the application to install"), ArgumentType::Positional, false },
            Argument{ ARG_MANIFEST, LOCME("The path to the manifest of the application to install"), ArgumentType::Standard, false },
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

    void InstallCommand::ExecuteInternal(Invocation& inv, std::ostream& out) const
    {
        if (inv.Contains(ARG_MANIFEST))
        {
            std::string manifest = *(inv.GetArg(ARG_MANIFEST));
            Manifest::Manifest packageManifest = Manifest::Manifest::CreateFromPath(manifest);
            InstallFlow packageInstall(packageManifest, out);
            packageInstall.Install();
        }
        else
        {
            out << "Not supported!" << std::endl;
        }
    }

    void InstallCommand::ValidateArguments(Invocation& inv) const
    {
        Command::ValidateArguments(inv);

        if (!inv.Contains(ARG_APPLICATION) && !inv.Contains(ARG_MANIFEST))
        {
            throw CommandException(LOCME("Required argument not provided"), ARG_APPLICATION);
        }
    }
}
