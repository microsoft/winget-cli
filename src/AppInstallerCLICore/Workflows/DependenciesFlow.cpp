// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "DependenciesFlow.h"
#include "ManifestComparator.h"

namespace AppInstaller::CLI::Workflow
{
    using namespace AppInstaller::Repository;
    using namespace Manifest;

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
            info << Resource::StringId(m_messageId) << std::endl;

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
                        if (dependency.MinVersion) info << " [>= " << dependency.MinVersion.value().ToString() << "]";
                        info << std::endl;
                    });
            }

            if (dependencies.HasAnyOf(DependencyType::External))
            {
                info << "  - " << Resource::String::ExternalDependencies << std::endl;
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

            context.Add<Execution::Data::Dependencies>(allDependencies);
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
        // two options: have a package version or a manifest
        // try to get source from package version

        //source = empty
        //if PackageVersion then source = PV source
        //else if Source exists then fail  // if Source already open then this can't be an install -m or validate command
        //else then OpenSource; source = Source
        //    result = CreateComposite(installed, source)

        if (context.Contains(Execution::Data::PackageVersion))
        {
            const auto& packageVersion = context.Get<Execution::Data::PackageVersion>();
            // how to execute without progress? should we?
            const auto& installedSource = context.Reporter.ExecuteWithProgress(std::bind(Repository::OpenPredefinedSource, PredefinedSource::Installed, std::placeholders::_1), true);
            auto compositeSource = Repository::CreateCompositeSource(installedSource, packageVersion->GetSource());
            context.Add<Execution::Data::DependencySource>(compositeSource);
        }
        else
        {
            // TODO set up something like --dependency-source, open source passed by parameter (from source name)
            context <<
                Workflow::OpenSource <<
                Workflow::OpenCompositeSource(Repository::PredefinedSource::Installed);
            context.Add<Execution::Data::DependencySource>(context.Get<Execution::Data::Source>());
        }
    }

    void BuildPackageDependenciesGraph(Execution::Context& context)
    {
        auto info = context.Reporter.Info();
        const auto& rootManifest = context.Get<Execution::Data::Manifest>();
        Dependency rootDependency = Dependency(DependencyType::Package, rootManifest.Id, rootManifest.Version);
        
        std::vector<Dependency> toCheck;
        std::map<Dependency, std::vector<Dependency>> dependencyGraph; //(?) value should be a set instead of a vector?
        const auto& rootInstaller = context.Get<Execution::Data::Installer>();
        if (rootInstaller)
        {
            context.Add<Execution::Data::Dependencies>(rootInstaller->Dependencies); // to use in report
            // TODO remove this ^ if we are reporting dependencies somewhere else while installing/managing them
            dependencyGraph[rootDependency] = std::vector<Dependency>(); 
            rootInstaller->Dependencies.ApplyToType(DependencyType::Package, [&](Dependency dependency)
                {
                    toCheck.push_back(dependency);
                    dependencyGraph[dependency] = std::vector<Dependency>();
                    dependencyGraph[rootDependency].push_back(dependency);

                });
        }
        else
        {
            info << "no intaller found" << std::endl;
            //TODO warn user and raise error, this should not happen as the workflow should fail before reaching this.
        }

        context << OpenDependencySource;
        if (!context.Contains(Execution::Data::DependencySource))
        {
            context.Reporter.Info() << "dependency source not found" << std::endl; //TODO localize message
            return; //TODO terminate with error once we can get source for dependencies, when a manifest was passed
            //AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_SOURCE_DATA_MISSING); // TODO create a new error code?
        }

        const auto& source = context.Get<Execution::Data::DependencySource>();
        std::map<string_t, string_t> failedPackages;
        std::vector<Dependency> alreadyInstalled;

        for (int i = 0; i < toCheck.size(); ++i)
        {
            auto dependencyNode = toCheck.at(i);

            SearchRequest searchRequest;
            searchRequest.Filters.emplace_back(PackageMatchFilter(PackageMatchField::Id, MatchType::CaseInsensitive, dependencyNode.Id));
            const auto& matches = source->Search(searchRequest).Matches;

            if (!matches.empty())
            {
                const auto& match = matches.at(0);
                if (matches.size() > 1) {
                    failedPackages[dependencyNode.Id] = "Too many matches"; //TODO localize all errors
                    continue;
                }

                const auto& package = match.Package;
                if (package->GetInstalledVersion() && dependencyNode.IsVersionOk(package->GetInstalledVersion()->GetManifest().Version))
                {
                    alreadyInstalled.push_back(dependencyNode);
                }
                else
                {
                    const auto& packageVersion = package->GetLatestAvailableVersion();
                    if (!packageVersion) {
                        failedPackages[dependencyNode.Id] = "No package version found"; //TODO localize all errors
                        continue;
                    }

                    const auto& packageVersionManifest = packageVersion->GetManifest();
                    if (packageVersionManifest.Installers.empty()) {
                        failedPackages[dependencyNode.Id] = "No installers found"; //TODO localize all errors
                        continue;
                    }

                    if (!dependencyNode.IsVersionOk(packageVersionManifest.Version))
                    {
                        failedPackages[dependencyNode.Id] = "Minimum required version not available"; //TODO localize all errors
                        continue;
                    }
                    
                    const auto& matchInstaller = SelectInstallerFromMetadata(context, packageVersion->GetMetadata());
                    if (!matchInstaller)
                    {
                        failedPackages[dependencyNode.Id] = "No installer found"; //TODO localize all errors
                        continue;
                    }

                    const auto& matchDependencies = matchInstaller.value().Dependencies;

                    // TODO save installers for later maybe?
                    matchDependencies.ApplyToType(DependencyType::Package, [&](Dependency dependency)
                        {
                            dependencyGraph[dependencyNode].push_back(dependency);

                            auto search = dependencyGraph.find(dependency);
                            if (search == dependencyGraph.end()) // if not found
                            {
                                    toCheck.push_back(dependency);
                                    dependencyGraph[dependency] = std::vector<Dependency>();
                            }
                        });
                }
            }
            else
            {
                failedPackages[dependencyNode.Id] = "No matches"; //TODO localize all errors
                continue;
            }
        }
        auto installationOrder = std::vector<Dependency>();
        if (graphHasLoop(dependencyGraph, rootDependency, installationOrder))
        {
            info << "has loop" << std::endl;
            //TODO warn user and raise error
        }

        // TODO raise error for failedPackages (if there's at least one)

        info << "order: ";
        for (auto const& node : installationOrder)
        {
            info << node.Id << ", ";
        }
        info << std::endl;
    }

    // TODO make them iterative
    // is there a better way that this to check for loops?
    bool graphHasLoop(const std::map<Dependency, std::vector<Dependency>>& dependencyGraph, const Dependency& root, std::vector<Dependency>& order)
    {
        auto visited = std::set<Dependency>();
        visited.insert(root);
        if (hasLoopDFS(visited, root, dependencyGraph, order))
        {
            return true;
        }
        return false;
    }

    bool hasLoopDFS(std::set<Dependency> visited, const Dependency& node, const std::map<Dependency, std::vector<Dependency>>& dependencyGraph, std::vector<Dependency>& order)
    {
        visited.insert(node);
        for (const auto& adjacent : dependencyGraph.at(node))
        {
            auto search = visited.find(adjacent);
            if (search == visited.end()) // if not found
            {
                if (hasLoopDFS(visited, adjacent, dependencyGraph, order))
                {
                    return true;
                }
            }
            else 
            {
                return true;
            }
        }
        
        if (std::find(order.begin(), order.end(), node) == order.end()) 
        {
            order.push_back(node);
        }

        return false;
    }
}