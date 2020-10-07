// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"
#include "WorkflowBase.h"

namespace AppInstaller::CLI::Workflow
{
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