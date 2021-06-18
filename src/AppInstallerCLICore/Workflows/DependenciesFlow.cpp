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
            for (const auto& dependency : dependencies.dependencies) { //TODO change for lambda function called inside DepList
                if (dependency.Type == Manifest::DependencyType::WindowsFeature) info << "  " << dependency.Id << std::endl;
            }

            if (dependencies.HasAnyOf(Manifest::DependencyType::WindowsLibraries)) info << "    - Windows Libraries: " << std::endl;
            for (const auto& dependency : dependencies.dependencies) { //TODO change for lambda function called inside DepList
                if (dependency.Type == Manifest::DependencyType::WindowsLibraries) info << "  " << dependency.Id << std::endl;
            }

            if (dependencies.HasAnyOf(Manifest::DependencyType::Package)) info << "    - Package: " << std::endl;
            for (const auto& dependency : dependencies.dependencies) { //TODO change for lambda function called inside DepList
                if (dependency.Type == Manifest::DependencyType::Package)
                {
                    info << "  " << dependency.Id;
                    if (dependency.MinVersion) info << " [>= " << dependency.MinVersion.value() << "]";
                    info << std::endl;
                }
            }

            if (dependencies.HasAnyOf(Manifest::DependencyType::External)) info << "    - External: " << std::endl;
            for (const auto& dependency : dependencies.dependencies) { //TODO change for lambda function called inside DepList
                if (dependency.Type == Manifest::DependencyType::External) info << "  " << dependency.Id << std::endl;
            }
        }
    }
}