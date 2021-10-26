// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "DependenciesFlow.h"
#include "ManifestComparator.h"
#include "InstallFlow.h"
#include "winget\DependenciesGraph.h"
#include "DependencyNodeProcessor.h"

using namespace AppInstaller::Repository;
using namespace AppInstaller::Manifest;

namespace AppInstaller::CLI::Workflow
{
    void ReportDependencies::operator()(Execution::Context& context) const
    {
        if (!Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Dependencies))
        {
            return;
        }
        auto info = context.Reporter.Info();
        
        const auto& dependencies = context.Get<Execution::Data::Dependencies>();
        if (dependencies.HasAny())
        {
            info << m_messageId << std::endl;

            if (dependencies.HasAnyOf(DependencyType::WindowsFeature))
            {
                info << "  - " << Resource::String::WindowsFeaturesDependencies << std::endl;
                dependencies.ApplyToType(DependencyType::WindowsFeature, [&info](Dependency dependency) {info << "      " << dependency.Id << std::endl; });
            }

            if (dependencies.HasAnyOf(DependencyType::WindowsLibrary))
            {
                info << "  - " << Resource::String::WindowsLibrariesDependencies << std::endl;
                dependencies.ApplyToType(DependencyType::WindowsLibrary, [&info](Dependency dependency) {info << "      " << dependency.Id << std::endl; });
            }

            if (dependencies.HasAnyOf(DependencyType::Package))
            {
                info << "  - " << Resource::String::PackageDependencies << std::endl;
                dependencies.ApplyToType(DependencyType::Package, [&info](Dependency dependency)
                    {
                        info << "      " << dependency.Id;
                        if (dependency.MinVersion)
                        {
                            info << " [>= " << dependency.MinVersion.value().ToString() << "]";
                        }
                        info << std::endl;
                    });
            }

            if (dependencies.HasAnyOf(DependencyType::External))
            {
                context.Reporter.Warn() << "  - " << Resource::String::ExternalDependencies << std::endl;
                dependencies.ApplyToType(DependencyType::External, [&info](Dependency dependency) {info << "      " << dependency.Id << std::endl; });
            }
        }
    }

    void GetInstallersDependenciesFromManifest(Execution::Context& context) {
        if (Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Dependencies))
        {
            const auto& manifest = context.Get<Execution::Data::Manifest>();
            DependencyList allDependencies;

            for (const auto& installer : manifest.Installers)
            {
                allDependencies.Add(installer.Dependencies);
            }

            context.Add<Execution::Data::Dependencies>(std::move(allDependencies));
        }
    }

    void GetDependenciesFromInstaller(Execution::Context& context)
    {
        if (Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Dependencies))
        {
            const auto& installer = context.Get<Execution::Data::Installer>();
            if (installer)
            {
                context.Add<Execution::Data::Dependencies>(installer->Dependencies);
            }
        }
    }

    void GetDependenciesInfoForUninstall(Execution::Context& context)
    {
        if (Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Dependencies))
        {
            // TODO make best effort to get the correct installer information, it may be better to have a record of installations and save the correct installers
            context.Add<Execution::Data::Dependencies>(DependencyList()); // sending empty list of dependencies for now
        }
    }

    void OpenDependencySource(Execution::Context& context)
    {
        if (context.Contains(Execution::Data::PackageVersion))
        {
            const auto& packageVersion = context.Get<Execution::Data::PackageVersion>();
            context.Add<Execution::Data::DependencySource>(std::const_pointer_cast<Repository::ISource>(packageVersion->GetSource()));
            context <<
                Workflow::OpenCompositeSource(Repository::PredefinedSource::Installed, true);
        }
        else
        { // install from manifest requires --dependency-source to be set
            context <<
                Workflow::OpenSource(true) <<
                Workflow::OpenCompositeSource(Repository::PredefinedSource::Installed, true);
        }
    }


    void ManagePackageDependencies::operator()(Execution::Context& context) const
    {
        if (!Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Dependencies))
        {
            return;
        }

        auto info = context.Reporter.Info();
        auto error = context.Reporter.Error();
        const auto& rootManifest = context.Get<Execution::Data::Manifest>();

        Dependency rootAsDependency = Dependency(DependencyType::Package, rootManifest.Id, rootManifest.Version);
        
        const auto& rootInstaller = context.Get<Execution::Data::Installer>();
        const auto& rootDependencies = rootInstaller->Dependencies;

        if (rootDependencies.Empty())
        {
            // If there's no dependencies there's nothing to do aside of logging the outcome
            return;
        }

        info << Resource::String::DependenciesFlowInstall << std::endl;

        context << OpenDependencySource;
        if (context.IsTerminated())
        {
            info << Resource::String::DependenciesFlowSourceNotFound << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INTERNAL_ERROR); 
        }

        
        std::map<string_t, Execution::PackageToInstall> idToPackageMap;
        bool foundError = false;
        DependencyGraph dependencyGraph(rootAsDependency, rootDependencies, 
            [&](Dependency node) {
                DependencyNodeProcessor nodeProcessor(context);
                
                auto result = nodeProcessor.EvaluateDependencies(node);
                DependencyList list = nodeProcessor.GetDependencyList();
                foundError = foundError || (result == DependencyNodeProcessorResult::Error);

                if (result == DependencyNodeProcessorResult::Success)
                {
                    Execution::PackageToInstall packageToInstall{
                        std::move(nodeProcessor.GetPackageLatestVersion()),
                        std::move(nodeProcessor.GetPackageInstalledVersion()),
                        std::move(nodeProcessor.GetManifest()),
                        std::move(nodeProcessor.GetPreferredInstaller()) };
                    idToPackageMap.emplace(node.Id, std::move(packageToInstall));
                };

                return list;
            });

        dependencyGraph.BuildGraph();

        if (foundError)
        {
            error << Resource::String::DependenciesManagementExitMessage << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INSTALL_MISSING_DEPENDENCY);
        }

        if (dependencyGraph.HasLoop())
        {
            context.Reporter.Warn() << Resource::String::DependenciesFlowContainsLoop;
        }

        const auto& installationOrder = dependencyGraph.GetInstallationOrder();

        std::vector<Execution::PackageToInstall> installers;

        for (auto const& node : installationOrder)
        {            
            auto itr = idToPackageMap.find(node.Id);
            // if the package was already installed (with a useful version) or is the root
            // then there will be no installer for it on the map.
            if (itr != idToPackageMap.end())
            {
                installers.push_back(std::move(itr->second));
            }
        }
        
        // Install dependencies in the correct order
        context.Add<Execution::Data::PackagesToInstall>(installers);
        context << Workflow::InstallMultiplePackages(m_dependencyReportMessage, APPINSTALLER_CLI_ERROR_INSTALL_DEPENDENCIES, {}, false, true);
    }
}