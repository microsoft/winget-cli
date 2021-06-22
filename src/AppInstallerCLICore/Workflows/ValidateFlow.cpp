// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ValidateFlow.h"

namespace AppInstaller::CLI::Workflow
{
    void GetDependenciesFromManifest(Execution::Context& context) {
        if (Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Dependencies))
        {
            const auto& manifest = context.Get<Execution::Data::Manifest>();
            Manifest::DependencyList allDependencies;
            
            for (const auto& installer : manifest.Installers) 
            { 
                allDependencies.Add(installer.Dependencies); 
            }

            context.Add<Execution::Data::Dependencies>(allDependencies);
            context.Reporter.Info() << Resource::String::ValidateCommandReportDependencies << std::endl;
        }
    }
}


