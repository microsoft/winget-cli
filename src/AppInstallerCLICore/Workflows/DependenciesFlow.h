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

    // Builds the dependency graph and creates the sub contexts for each package dependency.
    // Required Args: None
    // Inputs: Manifest, Installer and DependencySource
    // Outputs: Dependencies
    struct CreateDependencySubContexts : public WorkflowTask
    {
        CreateDependencySubContexts(
            AppInstaller::StringResource::StringId dependencyReportMessage) :
            WorkflowTask("CreateDependencySubContexts"),
            m_dependencyReportMessage(dependencyReportMessage) {}

        void operator()(Execution::Context& context) const override;

        private:
            AppInstaller::StringResource::StringId m_dependencyReportMessage;
    };

    // Sets up the source used to get the dependencies.
    // Required Args: None
    // Inputs: PackageVersion, Manifest
    // Outputs: DependencySource
    void OpenDependencySource(Execution::Context& context);

    // Enables the Windows Feature dependencies.
    // Required Args: None
    // Inputs: Manifest, Installer
    // Outputs: None
    void EnableWindowsFeaturesDependencies(Execution::Context& context);
}