// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Shows information about dependencies.
    // Required Args: message to use at the begining, before outputting dependencies
    // Inputs: Manifest
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
    // Outputs: None
    void GetInstallersDependenciesFromManifest(Execution::Context& context);

    // Gathers package dependencies information from installer.
    // Required Args: None
    // Inputs: Installer
    // Outputs: None
    void GetDependenciesFromInstaller(Execution::Context& context);
}