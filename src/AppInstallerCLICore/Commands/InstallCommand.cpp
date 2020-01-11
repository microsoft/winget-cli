// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallCommand.h"
#include "Localization.h"
#include "..\AppInstallerRepositoryCore\Manifest\Manifest.h"
#include "..\Workflows\InstallFlow.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Workflow;

namespace AppInstaller::CLI
{
    std::vector<Argument> InstallCommand::GetArguments() const
    {
        return {
            Argument{ "application", LOCME("The name of the application to install"), ArgumentType::Positional, false },
            Argument{ "manifest", LOCME("The path to the manifest of the application to install"), ArgumentType::Standard, false },
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
        if (inv.Contains("manifest"))
        {
            for (auto const& manifest : *(inv.GetArgs("manifest")))
            {
                try
                {
                    Manifest::Manifest packageManifest = Manifest::Manifest::CreatePackageManifestFromFile(manifest);
                    InstallFlow packageInstall(packageManifest);
                    packageInstall.Install();
                }
                catch (std::exception& e)
                {
                    AICLI_LOG(CLI, Error, << "Failed to install package using manifest file at: " << manifest);
                    continue;
                }
            }
        }
    }
}
