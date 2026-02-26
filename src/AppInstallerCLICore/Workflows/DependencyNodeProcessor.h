// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/RepositorySearch.h>
#include "ExecutionContext.h"
#include <winget/ManifestCommon.h>

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;

namespace AppInstaller::CLI::Workflow 
{
    enum DependencyNodeProcessorResult
    {
        Error,
        Success,
        Skipped,
    };

    struct DependencyNodeProcessor
    {
        DependencyNodeProcessor(Execution::Context& context);

        DependencyNodeProcessorResult EvaluateDependencies(Dependency& dependencyNode);

        DependencyList GetDependencyList() { return m_dependenciesList; }

        std::shared_ptr<IPackageVersion>  GetPackageLatestVersion() { return m_nodePackageLatestVersion; }

        std::shared_ptr<IPackageVersion> GetPackageInstalledVersion() { return m_nodePackageInstalledVersion; }

        Manifest::Manifest GetManifest() { return m_nodeManifest;  }

        Manifest::ManifestInstaller GetPreferredInstaller() { return m_installer; }

    private:
        Execution::Context& m_context;
        DependencyList m_dependenciesList;
        std::shared_ptr<IPackageVersion> m_nodePackageLatestVersion;
        std::shared_ptr<IPackageVersion> m_nodePackageInstalledVersion;
        Manifest::ManifestInstaller m_installer;
        Manifest::Manifest m_nodeManifest;
    };
}