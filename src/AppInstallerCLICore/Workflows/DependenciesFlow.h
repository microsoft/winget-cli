// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Shows information about dependencies.
    // Required Args: message to use at the beginning, before outputting dependencies
    // Inputs: Dependencies
    // Outputs: None
    struct ReportDependencies : public WorkflowTask
    {
        ReportDependencies(AppInstaller::StringResource::StringId messageId) :
            WorkflowTask("ReportDependencies"), m_messageId(messageId) {}

        void operator()(Execution::Context& context) const override;

    private:
        AppInstaller::StringResource::StringId m_messageId;
    };

    // Gathers all installers dependencies from manifest.
    // Required Args: None
    // Inputs: Manifest
    // Outputs: Dependencies
    void GetInstallersDependenciesFromManifest(Execution::Context& context);

    // Gathers package dependencies information from installer.
    // Required Args: None
    // Inputs: Installer
    // Outputs: Dependencies
    void GetDependenciesFromInstaller(Execution::Context& context);

    // TODO: 
    // Gathers dependencies information for the uninstall command.
    // Required Args: None
    // Inputs: None
    // Outputs: Dependencies
    void GetDependenciesInfoForUninstall(Execution::Context& context);

    // Builds the dependency graph.
    // Required Args: None
    // Inputs: DependencySource
    // Outputs: Dependencies
    void BuildPackageDependenciesGraph(Execution::Context& context);

    // Sets up the source used to get the dependencies.
    // Required Args: None
    // Inputs: PackageVersion, Manifest
    // Outputs: DependencySource
    void OpenDependencySource(Execution::Context& context);

    struct DependencyGraph
    {
        DependencyGraph(AppInstaller::Manifest::Dependency root, 
            AppInstaller::Manifest::DependencyList rootDependencies,
            std::function<AppInstaller::Manifest::DependencyList(const AppInstaller::Manifest::Dependency&)> infoFunction) : m_root(root), getDependencies(infoFunction)
        {
            adjacents[m_root] = std::vector<AppInstaller::Manifest::Dependency>();
            toCheck = std::vector<AppInstaller::Manifest::Dependency>();
            rootDependencies.ApplyToType(AppInstaller::Manifest::DependencyType::Package, [&](AppInstaller::Manifest::Dependency dependency)
                {
                    toCheck.push_back(dependency);
                    AddNode(dependency);
                    AddAdjacent(root, dependency);
                });
        }

        void BuildGraph()
        {
            if (toCheck.empty())
            {
                return;
            }

            for (int i = 0; i < toCheck.size(); ++i)
            {
                auto node = toCheck.at(i);

                const auto& nodeDependencies = getDependencies(node); //TODO add error stream so we can report back

                nodeDependencies.ApplyToType(AppInstaller::Manifest::DependencyType::Package, [&](AppInstaller::Manifest::Dependency dependency)
                    {
                        if (!HasNode(dependency))
                        {
                            toCheck.push_back(dependency);
                            AddNode(dependency);
                        }

                        AddAdjacent(node, dependency);
                    });
            }

            CheckForLoopsAndGetOrder();
        }

        void AddNode(AppInstaller::Manifest::Dependency node)
        {
            adjacents[node] = std::vector<AppInstaller::Manifest::Dependency>();

        }

        void AddAdjacent(AppInstaller::Manifest::Dependency node, AppInstaller::Manifest::Dependency adjacent)
        {
            adjacents[node].push_back(adjacent);
        }

        bool HasNode(AppInstaller::Manifest::Dependency dependency)
        {
            auto search = adjacents.find(dependency);
            return search != adjacents.end();
        }

        bool HasLoop()
        {
            return hasLoop;
        }
        
        // TODO make CheckForLoops and HasLoopDFS iterative
        void CheckForLoopsAndGetOrder()
        {
            installationOrder = std::vector<AppInstaller::Manifest::Dependency>();
            std::set<AppInstaller::Manifest::Dependency> visited;
            hasLoop = HasLoopDFS(visited, m_root);
        }

        std::vector<AppInstaller::Manifest::Dependency> GetInstallationOrder()
        {
            return installationOrder;
        }

        //-- only for debugging
        void PrintOrder(AppInstaller::CLI::Execution::OutputStream info)
        {
            info << "order: ";
            for (auto const& node : installationOrder)
            {
                info << node.Id << ", ";
            }
            info << std::endl;
        }

    private:
        bool HasLoopDFS(std::set<AppInstaller::Manifest::Dependency> visited, const AppInstaller::Manifest::Dependency& node)
        {
            visited.insert(node);
            for (const auto& adjacent : adjacents.at(node))
            {
                auto search = visited.find(adjacent);
                if (search == visited.end()) // if not found
                {
                    if (HasLoopDFS(visited, adjacent))
                    {
                        return true;
                    }
                }
                else
                {
                    return true;
                }
            }

            if (std::find(installationOrder.begin(), installationOrder.end(), node) == installationOrder.end())
            {
                installationOrder.push_back(node);
            }

            return false;
        }

        AppInstaller::Manifest::Dependency m_root;
        std::map<AppInstaller::Manifest::Dependency, std::vector<AppInstaller::Manifest::Dependency>> adjacents; //(?) value should be a set instead of a vector?
        std::function<AppInstaller::Manifest::DependencyList(const AppInstaller::Manifest::Dependency&)> getDependencies;
        
        bool hasLoop;
        std::vector<AppInstaller::Manifest::Dependency> installationOrder;
        
        std::vector<AppInstaller::Manifest::Dependency> toCheck;
        std::map<AppInstaller::Manifest::string_t, AppInstaller::Manifest::string_t> failedPackages;
    };
}