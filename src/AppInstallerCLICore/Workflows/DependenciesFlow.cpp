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
        if (dependencies.HasAny())
        {
            auto info = context.Reporter.Info();

            if (dependencies.HasAnyOf(Manifest::DependencyType::WindowsFeature)) info << "    - Windows Features: " << std::endl;
            for (const auto& dep : dependencies.dependencies) { //TODO change for lambda function called inside DepList
                if (dep.Type == Manifest::DependencyType::WindowsFeature) info << "  " << dep.Id << std::endl;
            }

            if (dependencies.HasAnyOf(Manifest::DependencyType::WindowsLibraries)) info << "    - Windows Libraries: " << std::endl;
            for (const auto& dep : dependencies.dependencies) { //TODO change for lambda function called inside DepList
                if (dep.Type == Manifest::DependencyType::WindowsLibraries) info << "  " << dep.Id << std::endl;
            }

            if (dependencies.HasAnyOf(Manifest::DependencyType::Package)) info << "    - Package: " << std::endl;
            for (const auto& dep : dependencies.dependencies) { //TODO change for lambda function called inside DepList
                if (dep.Type == Manifest::DependencyType::Package)
                {
                    info << "  " << dep.Id;
                    if (dep.MinVersion) info << " [>= " << dep.MinVersion.value() << "]";
                    info << std::endl;
                }
            }

            if (dependencies.HasAnyOf(Manifest::DependencyType::External)) info << "    - External: " << std::endl;
            for (const auto& dep : dependencies.dependencies) { //TODO change for lambda function called inside DepList
                if (dep.Type == Manifest::DependencyType::External) info << "  " << dep.Id << std::endl;
            }
        }
    }
}