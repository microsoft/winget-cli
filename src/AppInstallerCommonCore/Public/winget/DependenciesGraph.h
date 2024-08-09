// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winget/ManifestCommon.h"

namespace AppInstaller::Manifest 
{
    struct DependencyGraph
    {
        // this constructor was intended for use during installation flow (we already have installer dependencies and there's no need to search the source again)
        DependencyGraph(const Dependency& root, const DependencyList& rootDependencies,
            std::function<const DependencyList(const Dependency&)> infoFunction);

        DependencyGraph(const Dependency& root, std::function<const DependencyList(const Dependency&)> infoFunction);

        void BuildGraph();

        void AddNode(const Dependency& node);

        void AddAdjacent(const Dependency& node, const Dependency& adjacent);

        bool HasNode(const Dependency& dependency);

        bool HasLoop();

        void CheckForLoopsAndGetOrder();

        std::vector<Dependency> GetInstallationOrder();

    private:
        // TODO make this function iterative
        bool HasLoopDFS(std::set<Dependency> visited, const Dependency& node);

        const Dependency& m_root;
        std::map<Dependency, std::set<Dependency>> m_adjacents;
        std::function<const DependencyList(const Dependency&)> getDependencies;
        bool m_HasLoop = false;
        bool m_rootDependencyEvaluated = false;
        std::vector<Dependency> m_installationOrder;
        std::vector<Dependency> m_toCheck;
    };
}
