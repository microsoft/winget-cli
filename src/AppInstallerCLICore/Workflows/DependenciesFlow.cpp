// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "DependenciesFlow.h"

namespace AppInstaller::CLI::Workflow
{
    void ReportDependencies(Execution::Context& context)
    {
        if (!Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Dependencies)) return;
        
        const auto& dependencies = context.Get<Execution::Data::Dependencies>();
        if (dependencies && dependencies->HasAny())
        {
            auto info = context.Reporter.Info();

            info << "    - Windows Features: ";
            for (const auto& dep : dependencies->dependencies) { //TODO change for lambda function called inside DepList
                if (dep.Type == Manifest::DependencyType::WindowsFeature) info << "  " << dep << std::endl;
            }

            info << "    - Windows Libraries: ";
            for (const auto& dep : dependencies->dependencies) { //TODO change for lambda function called inside DepList
                if (dep.Type == Manifest::DependencyType::WindowsLibraries) info << "  " << dep << std::endl;
            }

            info << "    - Package: ";
            for (const auto& dep : dependencies->dependencies) { //TODO change for lambda function called inside DepList
                if (dep.Type == Manifest::DependencyType::Package)
                {
                    info << "  " << dep.Id;
                    if (dep.MinVersion) info << " [>= " << dep.MinVersion << "]";
                    info << std::endl;
                }
            }

            info << "    - External: ";
            for (const auto& dep : dependencies->dependencies) { //TODO change for lambda function called inside DepList
                if (dep.Type == Manifest::DependencyType::External) info << "  " << dep << std::endl;
            }
        }
    }
}