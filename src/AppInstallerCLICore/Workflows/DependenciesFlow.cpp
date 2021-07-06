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
                        if (dependency.MinVersion) info << " [>= " << dependency.MinVersion.value() << "]";
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
        const auto& packageVersion = context.Get<Execution::Data::PackageVersion>();

        if (packageVersion)
        {
            // TODO open composite source: package + installed
            // how to execute without progress?
            //auto installedSource = context.Reporter.ExecuteWithProgress(std::bind(Repository::OpenPredefinedSource, PredefinedSource::Installed, std::placeholders::_1), true);
            //auto compositeSource = Repository::CreateCompositeSource(installedSource, packageVersion->GetSource());
            auto compositeSource = packageVersion->GetSource();
            context.Add<Execution::Data::DependencySource>(compositeSource);
        }
        else
        {
            // TODO question to John: can/should we do nothing for local manifests? or set up something like --dependency-source
            const auto& manifest = context.Get<Execution::Data::Manifest>();
        }
    }

    void BuildPackageDependenciesGraph(Execution::Context& context)
    {
        context << OpenDependencySource;
        const auto& source = context.Get<Execution::Data::DependencySource>();

        std::vector<Dependency> toCheck;

        // should this dictionary have Dependency as key? (to have min version) in this case Dependency should implement equal
        std::map<string_t, std::vector<Dependency>> dependencyGraph; // < package Id, dependencies >
        std::map<string_t, std::vector<string_t>> inverseDependencyGraph; // < package Id, packages that depends on this one>

        const auto& installer = context.Get<Execution::Data::Installer>();
        if (installer)
        {
            dependencyGraph[installer->ProductId] = std::vector<Dependency>(); // ProductId is the same Id as the one used by Dependencies?
            installer->Dependencies.ApplyToType(DependencyType::Package, [&](Dependency dependency)
                {
                    toCheck.push_back(dependency);
                    dependencyGraph[dependency.Id] = std::vector<Dependency>();
                    dependencyGraph[installer->ProductId].push_back(dependency);

                    inverseDependencyGraph[dependency.Id] = std::vector<string_t>{ installer->ProductId };
                });
        } // TODO fail otherwise

        for (const auto& dependency : toCheck)
        {
            // search the package source+installed to see if the dep exists
            SearchRequest searchRequest;
            searchRequest.Filters.emplace_back(PackageMatchFilter(PackageMatchField::Id, MatchType::CaseInsensitive, dependency.Id));
            const auto& matches = source->Search(searchRequest).Matches;

            if (!matches.empty())
            {
                const auto& match = matches.at(0); // What to do if there's more than one? TODO should not happen (report to the user)
                const auto& package = match.Package;
                if (!package->GetInstalledVersion())
                {
                    const auto& packageVersion = package->GetLatestAvailableVersion();
                    //how to choose best installer, should I use SelectInstaller or not?
                    const auto& installer = packageVersion->GetManifest().Installers.at(0); // fail if there's no installer?
                    const auto& dependencies = installer.Dependencies;
                    auto packageId = installer.ProductId; // ProductId is the same Id as the one used by Dependencies?

                    // TODO save installers for later maybe?
                    dependencies.ApplyToType(DependencyType::Package, [&](Dependency dependency)
                        {
                            dependencyGraph[packageId].push_back(dependency); 

                            auto search = dependencyGraph.find(dependency.Id);
                            if (search == dependencyGraph.end()) // if not found
                            {
                                    toCheck.push_back(dependency);
                                    dependencyGraph[dependency.Id] = std::vector<Dependency>();

                                    inverseDependencyGraph[dependency.Id] = std::vector<string_t>{ packageId };
                            } 
                            else
                            {
                                // we only need to check for loops if the dependency already existed, right?
                                // should we have an inverse map? i.e., < id, packages that depend on this one >
                                // that can make searching for loops easier

                                inverseDependencyGraph[dependency.Id].push_back(packageId);
                                bool hasLoop = false;
                                auto searchLoop = inverseDependencyGraph[dependency.Id];

                                
                            }
                        });
                }
            }
        }

        context.Reporter.Info() << "---" << std::endl;

    }
}