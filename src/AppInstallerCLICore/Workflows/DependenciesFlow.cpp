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
        if (!context.Contains(Execution::Data::DependencySource))
        {
            info << Resource::String::DependenciesFlowSourceNotFound << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INTERNAL_ERROR); 
        }

        const auto& source = context.Get<Execution::Data::DependencySource>();
        std::map<string_t, Execution::PackageToInstall> idToPackageMap;
        bool foundError = false;
        DependencyGraph dependencyGraph(rootAsDependency, rootDependencies, 
            [&](Dependency node) {
                auto info = context.Reporter.Info();

                SearchRequest searchRequest;
                searchRequest.Filters.emplace_back(PackageMatchFilter(PackageMatchField::Id, MatchType::CaseInsensitive, node.Id));
                const auto& matches = source->Search(searchRequest).Matches;

                if (!matches.empty())
                {
                    if (matches.size() > 1) 
                    {
                        error << Resource::String::DependenciesFlowSourceTooManyMatches;
                        AICLI_LOG(CLI, Error, << "Too many matches for package " << Utility::Normalize(node.Id));
                        foundError = true;
                        return DependencyList();
                    }
                    const auto& match = matches.at(0);

                    const auto& package = match.Package;

                    auto packageId = package->GetProperty(PackageProperty::Id);
                    auto installedVersion = package->GetInstalledVersion();

                    if (installedVersion && node.IsVersionOk(installedVersion->GetManifest().Version))
                    {
                        // return empty dependency list,
                        // as we won't keep searching for dependencies for installed packages
                        return DependencyList(); 
                    }

                    std::shared_ptr<IPackageVersion> latestVersion = package->GetLatestAvailableVersion();
                    if (!latestVersion) 
                    {
                        error << Resource::String::DependenciesFlowPackageVersionNotFound;
                        AICLI_LOG(CLI, Error, << "Latest available version not found for package " << Utility::Normalize(packageId));
                        foundError = true;
                        return DependencyList();
                    }

                    auto manifest = latestVersion->GetManifest();
                    if (manifest.Installers.empty()) {
                        error << Resource::String::DependenciesFlowNoInstallerFound;
                        AICLI_LOG(CLI, Error, << "Installer not found for manifest " << Utility::Normalize(manifest.Id) << " with version" << Utility::Normalize(manifest.Version));
                        foundError = true;
                        return DependencyList();
                    }

                    if (!node.IsVersionOk(manifest.Version))
                    {
                        error << Resource::String::DependenciesFlowNoMinVersion;
                        AICLI_LOG(CLI, Error, << "No suitable min version found for package " << Utility::Normalize(manifest.Id));
                        foundError = true;
                        return DependencyList();
                    }

                    std::optional<AppInstaller::Manifest::ManifestInstaller> installer;

                    IPackageVersion::Metadata installationMetadata;
                    if (installedVersion)
                    {
                        installationMetadata = installedVersion->GetMetadata();
                    }

                    ManifestComparator manifestComparator(context, installationMetadata);
                    installer = manifestComparator.GetPreferredInstaller(manifest);

                    auto nodeDependencies = installer.value().Dependencies;

                    Execution::PackageToInstall packageToInstall{
                        std::move(latestVersion),
                        std::move(installedVersion),
                        std::move(manifest),
                        std::move(installer.value()) };

                    idToPackageMap.emplace(node.Id, packageToInstall);

                    return nodeDependencies;
                }
                else
                {
                    error << Resource::String::DependenciesFlowNoMatches;
                    foundError = true;
                    return DependencyList();
                }
            });

        dependencyGraph.BuildGraph(); // maybe it's better if it already does it on the constructor?

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
                installers.push_back(itr->second);
            }
        }
        
        if (foundError)
        {
            // ask PM what to do here.
            bool continueExec = context.Reporter.PromptForBoolResponse(Resource::String::DependenciesManagementError);
            if (!continueExec)
            {
                error << Resource::String::DependenciesManagementExitMessage << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INSTALL_MISSING_DEPENDENCY);
            }
        }

        // Install dependencies in the correct order
        context.Add<Execution::Data::PackagesToInstall>(installers);
        context << Workflow::InstallMultiplePackages(m_dependencyReportMessage, APPINSTALLER_CLI_ERROR_INSTALL_DEPENDENCIES, {}, false, true);
    }
}