// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Shows information on an application; this is only the information for package agreements
    // Required Args: None
    // Inputs: Manifest
    // Outputs: None
    void ShowAgreementsInfo(Execution::Context& context);

    // Shows information on an application.
    // Required Args: None
    // Inputs: Manifest, Installer
    // Outputs: None
    void ShowManifestInfo(Execution::Context& context);

    // Shows information on a package; this is only the information common to all installers.
    // Required Args: None
    // Inputs: Manifest
    // Outputs: None
    void ShowPackageInfo(Execution::Context& context);

    // Shows information on an installer
    // Required Args: None
    // Inputs: Installer
    // Outputs: None
    void ShowInstallerInfo(Execution::Context& context);

    // Shows the version for the specific manifest.
    // Required Args: None
    // Inputs: Manifest
    // Outputs: None
    void ShowManifestVersion(Execution::Context& context);

    // Composite flow that produces a manifest; either from one given on the command line or by searching.
    // Required Args: None
    // Inputs: None
    // Outputs: Manifest
    struct GetManifest : public WorkflowTask
    {
        GetManifest(bool considerPins) : WorkflowTask("GetManifest"), m_considerPins(considerPins) {}

        void operator()(Execution::Context& context) const override;

    private:
        bool m_considerPins;
    };
}