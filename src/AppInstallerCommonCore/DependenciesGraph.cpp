// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget\DependenciesGraph.h"

namespace AppInstaller::Manifest
{
    // this constructor was intented for use during installation flow (we already have installer dependencies and there's no need to search the source again)
    DependencyGraph::DependencyGraph(const Dependency& root, const DependencyList& rootDependencies,
        std::function<const DependencyList(const Dependency&)> infoFunction) : m_root(root), getDependencies(infoFunction)
    {
        m_adjacents[m_root] = std::set<Dependency>();
        m_toCheck = std::vector<Dependency>();
        rootDependencies.ApplyToType(DependencyType::Package, [&](Dependency dependency)
            {
                m_toCheck.push_back(dependency);
                AddNode(dependency);
                AddAdjacent(root, dependency);
            });
        m_rootDependencyEvaluated = true;
    }

    DependencyGraph::DependencyGraph(const Dependency& root, std::function<const DependencyList(const Dependency&)> infoFunction) : m_root(root), getDependencies(infoFunction)
    {
        m_adjacents[m_root] = std::set<Dependency>();
        m_toCheck = std::vector<Dependency>();
    }

    void DependencyGraph::BuildGraph()
    {
        if (!m_rootDependencyEvaluated) 
        {
            const DependencyList& rootDependencies = getDependencies(m_root);
            rootDependencies.ApplyToType(DependencyType::Package, [&](Dependency dependency)
                {
                    m_toCheck.push_back(dependency);
                    AddNode(dependency);
                    AddAdjacent(m_root, dependency);
                });
            m_rootDependencyEvaluated = true;
        }

        if (m_toCheck.empty())
        {
            return;
        }

        for (unsigned int i = 0; i < m_toCheck.size(); ++i)
        {
            auto node = m_toCheck.at(i);

            const auto& nodeDependencies = getDependencies(node);
            nodeDependencies.ApplyToType(DependencyType::Package, [&](Dependency dependency)
                {
                    if (!HasNode(dependency))
                    {
                        m_toCheck.push_back(dependency);
                        AddNode(dependency);
                    }

                    AddAdjacent(node, dependency);
                });
        }

        CheckForLoopsAndGetOrder();
    }

    void DependencyGraph::AddNode(const Dependency& node)
    {
        m_adjacents[node] = std::set<Dependency>();
    }

    void DependencyGraph::AddAdjacent(const Dependency& node, const Dependency& adjacent)
    {
        m_adjacents[node].emplace(adjacent);
    }

    bool DependencyGraph::HasNode(const Dependency& dependency)
    {
        auto search = m_adjacents.find(dependency);
        return search != m_adjacents.end();
    }

    bool DependencyGraph::HasLoop()
    {
        return m_HasLoop;
    }

    void DependencyGraph::CheckForLoopsAndGetOrder()
    {
        m_installationOrder = std::vector<Dependency>();
        std::set<Dependency> visited;
        m_HasLoop = HasLoopDFS(visited, m_root);
    }

    std::vector<Dependency> DependencyGraph::GetInstallationOrder()
    {
        return m_installationOrder;
    }

    // TODO make this function iterative
    bool DependencyGraph::HasLoopDFS(std::set<Dependency> visited, const Dependency& node)
    {
        bool loop = false;

        visited.insert(node);
        auto lAdjacents = m_adjacents.at(node);
        for (const auto& adjacent : m_adjacents.at(node))
        {
            auto search = visited.find(adjacent);
            if (search == visited.end()) // if not found
            {
                if (HasLoopDFS(visited, adjacent))
                {
                    loop = true;
                    // didn't break the loop to have a complete order at the end (even if a loop exists)
                }
            }
            else
            {
                loop = true;
                // didn't break the loop to have a complete order at the end (even if a loop exists)
            }
        }

        // Adding to have an order even if a loop is present
        if (std::find(m_installationOrder.begin(), m_installationOrder.end(), node) == m_installationOrder.end())
        {
            m_installationOrder.push_back(node);
        }

        return loop;
    }
}