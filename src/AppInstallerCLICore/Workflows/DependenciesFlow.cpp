// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "DependenciesFlow.h"
#include "ManifestComparator.h"
#include "InstallFlow.h"

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
        // two version: have a package version or a manifest
        // openDependencySource, new context data "DependencySource"
        // 
        //TODO change this, configure the source we want
        context << OpenSource;
        const auto& source = context.Get<Execution::Data::Source>();
        /*const auto& source = context.Get<Execution::Data::Package>()->GetLatestAvailableVersion()->GetSource();
        context.Add<Execution::Data::Source>(std::move(source));*/
    }

    void BuildPackageDependenciesGraph(Execution::Context& context)
    {
        
        const auto& dependencies = context.Get<Execution::Data::Dependencies>();

        //TODO change this, configure the source we want
        context << OpenSource;
        const auto& source = context.Get<Execution::Data::Source>();
        /*const auto& source = context.Get<Execution::Data::Package>()->GetLatestAvailableVersion()->GetSource();
        context.Add<Execution::Data::Source>(std::move(source));*/

        std::vector<Dependency> toCheck;
        dependencies.ApplyToType(DependencyType::Package, [&](Dependency dependency)
            {
                toCheck.push_back(dependency);
            });

        std::vector<PackagesAndInstallers> toInstall;
        for (const auto& dependency : toCheck)
        {
            // search the package to see if it exists
            SearchRequest searchRequest;
            searchRequest.Filters.emplace_back(PackageMatchFilter(PackageMatchField::Id, MatchType::CaseInsensitive, dependency.Id));
            const auto& matches = source->Search(searchRequest).Matches;

            if (!matches.empty())
            {
                const auto& match = matches.at(0); // What to do if there's more than one? should not happen (report to the user)
                const auto& installedVersion = match.Package->GetInstalledVersion();
                if (!installedVersion)
                {
                    // get manifest and installer, maybe use something like vector<PackagesAndInstallers> as in InstallMultiple?
                    //how to choose best installer, should I use SelectInstaller or not?
                    //toInstall.push_back(PackagesAndInstallers(installer, ));
                    
                    //match.Package->GetLatestAvailableVersion()->GetManifest().Installers.at(0).Dependencies;
                }
            }
        }

        context.Reporter.Info() << "---" << std::endl;

    }
}