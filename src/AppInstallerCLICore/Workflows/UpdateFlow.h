// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"
#include "WorkflowBase.h"

namespace AppInstaller::CLI::Workflow
{
    // Iterates through all available versions from a package and find latest applicable version
    // Required Args: bool indicating whether to report update not found
    // Inputs: InstalledPackageVersion?, Package
    // Outputs: Manifest?, Installer?
    struct SelectLatestApplicableVersion : public WorkflowTask
    {
        SelectLatestApplicableVersion(bool isSinglePackage) :
            WorkflowTask("SelectLatestApplicableUpdate"), m_isSinglePackage(isSinglePackage) {}

        void operator()(Execution::Context& context) const override;

    private:
        bool m_isSinglePackage;
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

    // Select single package version for install or upgrade
    // Required Args: bool indicating whether the flow is for upgrade
    // Inputs: Source, SearchResult
    // Outputs: None
    struct SelectSinglePackageVersionForInstallOrUpgrade : public WorkflowTask
    {
        SelectSinglePackageVersionForInstallOrUpgrade(bool isUpgrade) :
            WorkflowTask("SelectSinglePackageVersionForInstallOrUpgrade"), m_isUpgrade(isUpgrade) {}

        void operator()(Execution::Context& context) const override;

    private:
        mutable bool m_isUpgrade;
    };

    // Install or upgrade a single package
    // Required Args: bool indicating whether the flow is for upgrade
    // Inputs: Source
    // Outputs: None
    struct InstallOrUpgradeSinglePackage : public WorkflowTask
    {
        InstallOrUpgradeSinglePackage(bool isUpgrade) :
            WorkflowTask("InstallOrUpgradeSinglePackage"), m_isUpgrade(isUpgrade) {}

        void operator()(Execution::Context& context) const override;

    private:
        mutable bool m_isUpgrade;
    };
}