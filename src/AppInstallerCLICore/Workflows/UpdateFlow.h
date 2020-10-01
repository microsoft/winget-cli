// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"
#include "WorkflowBase.h"

namespace AppInstaller::CLI::Workflow
{
    // Composite flow that fetches installed package version and an update manifest; either from one given on the command line or by searching.
    // Required Args: None
    // Inputs: None
    // Outputs: Manifest, InstalledPackageVersion
    void GetUpdateManifestAndInstaller(Execution::Context& context);

    // Gets the update manifest from SearchResult. If Version arg is used, pick the version and ensure update applicability.
    // If Version arg is not used, a latest applicable update will be picked.
    // Required Args: None
    // Inputs: InstalledPackageVersion, SearchResult
    // Outputs: Manifest?, Installer?
    void GetUpdateManifestAndInstallerFromSearchResult(Execution::Context& context);

    // Iterates through all available versions from a package and find latest applicable update
    // Required Args: the package
    // Inputs: InstalledPackageVersion
    // Outputs: Manifest?, Installer?
    struct SelectLatestApplicableUpdate : public WorkflowTask
    {
        SelectLatestApplicableUpdate(const AppInstaller::Repository::IPackage& package) :
            WorkflowTask("SelectLatestApplicableUpdate"), m_package(package) {}

        void operator()(Execution::Context& context) const override;

    private:
        const AppInstaller::Repository::IPackage& m_package;
    };

    // Ensures the update package has higher version than installed
    // Required Args: None
    // Inputs: Manifest, InstalledPackageVersion
    // Outputs: None
    void EnsureUpdateVersionApplicable(Execution::Context& context);

    // Update all packages from SearchResult to latest if applicable
    // Required Args: None
    // Inputs: SearchResult
    // Outputs: None
    void UpdateAllApplicable(Execution::Context& context);
}