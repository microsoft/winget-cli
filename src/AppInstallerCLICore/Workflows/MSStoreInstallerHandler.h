// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::MSStore
{
    struct MSStoreDownloadFile;
}

// MSStoreInstallerHandler handles msstore installers.
namespace AppInstaller::CLI::Workflow
{
    // Deploys the Store app.
    // Required Args: None
    // Inputs: Installer
    // Outputs: None
    void MSStoreInstall(Execution::Context& context);

    // Updates the Store app if applicable.
    // Required Args: None
    // Inputs: Installer
    // Outputs: None
    void MSStoreUpdate(Execution::Context& context);

    // Attempt to repair the installation of an Store app that is already installed
    // Required Args: None
    // Inputs: Installer
    // Outputs: None
    void MSStoreRepair(Execution::Context& context);

    // Download a single MSStore package file
    // Required Args: None
    // Inputs: None
    // Outputs: None
    struct DownloadMSStorePackageFile : public WorkflowTask
    {
        DownloadMSStorePackageFile(const AppInstaller::MSStore::MSStoreDownloadFile& downloadFile, const std::filesystem::path& downloadDirectory) :
            WorkflowTask("DownloadMSStorePackageFile"), m_downloadFile(downloadFile), m_downloadDirectory(downloadDirectory) {}

        void operator()(Execution::Context& context) const override;

    private:
        const AppInstaller::MSStore::MSStoreDownloadFile& m_downloadFile;
        const std::filesystem::path& m_downloadDirectory;
    };

    // Downloads the Store app installer.
    // Required Args: None
    // Inputs: Installer
    // Outputs: None
    void MSStoreDownload(Execution::Context& context);

    // Ensure the Store app is not blocked by policy.
    // Required Args: None
    // Inputs: Installer
    // Outputs: None
    void EnsureStorePolicySatisfied(Execution::Context& context);

    // Change stub preference to full and installs full package if needed.
    // This should go into configuration flow once installing from the store is
    // moved out of this work flow.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void EnableConfiguration(Execution::Context& context);

    // Change stub preference to stub and installs stub package if needed.
    // This should go into configuration flow once installing from the store is
    // moved out of this work flow.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void DisableConfiguration(Execution::Context& context);
}
