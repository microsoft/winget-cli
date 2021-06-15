// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "DependenciesFlow.h"

namespace AppInstaller::CLI::Workflow
{
    void ReportDependencies(Execution::Context& context)
    {
        context <<
            Workflow::GetManifest <<
            Workflow::SelectInstaller;

        if (Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Dependencies))
        {
            const auto& installer = context.Get<Execution::Data::Installer>();
            if (installer && installer->Dependencies.HasAny())
            {
                auto info = context.Reporter.Info();
                auto dependencies = installer->Dependencies;

                auto windowsFeaturesDep = dependencies.WindowsFeatures;
                if (!windowsFeaturesDep.empty())
                {
                    info << "    - Windows Features: ";
                    for (size_t i = 0; i < windowsFeaturesDep.size(); i++)
                    {
                        info << "  " << windowsFeaturesDep[i] << std::endl;
                    }
                }

                auto windowsLibrariesDep = dependencies.WindowsLibraries;
                if (!windowsLibrariesDep.empty())
                {
                    info << "    - Windows Libraries: ";
                    for (size_t i = 0; i < windowsLibrariesDep.size(); i++)
                    {
                        info << "  " << windowsLibrariesDep[i] << std::endl;
                    }
                }

                auto packageDep = dependencies.PackageDependencies;
                if (!packageDep.empty())
                {
                    info << "    - Packages: ";
                    for (size_t i = 0; i < packageDep.size(); i++)
                    {
                        info << "  " << packageDep[i].Id;
                        if (!packageDep[i].MinVersion.empty()) info << " [>= " << packageDep[i].MinVersion << "]";
                        info << std::endl;
                    }
                }

                auto externalDependenciesDep = dependencies.ExternalDependencies;
                if (!externalDependenciesDep.empty())
                {
                    info << "    - Externals: ";
                    for (size_t i = 0; i < externalDependenciesDep.size(); i++)
                    {
                        info << "  " << externalDependenciesDep[i] << std::endl;
                    }
                }
            }
        }
    }
}