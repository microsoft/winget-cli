// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Composite flow that chooses what to do based on the installer type.
    // Required Args: None
    // Inputs: Manifest, Installer
    // Outputs: None
    void DownloadInstaller(Execution::Context& context);

    // Check if the desired installer has already been downloaded.
    // Required Args: None
    // Inputs: Manifest, Installer
    // Outputs: HashPair, InstallerPath (only if found)
    void CheckForExistingInstaller(Execution::Context& context);

    // Computes the download path for the installer file. Does nothing if already determined
    // Required Args: None
    // Inputs: Installer, Manifest
    // Outputs: InstallerPath
    void GetInstallerDownloadPath(Execution::Context& context);

    // Downloads the file referenced by the Installer.
    // This workflow task is also used by MSStoreDownload task.
    // Required Args: None
    // Inputs: Installer, Manifest
    // Outputs: HashPair, InstallerPath
    void DownloadInstallerFile(Execution::Context& context);

    // Computes the hash of the MSIX signature file.
    // Required Args: None
    // Inputs: Installer
    // Outputs: HashPair
    void GetMsixSignatureHash(Execution::Context& context);

    // Re-verify the installer hash. This is used in Com install commands where download and install are in separate phases.
    // Required Args: None
    // Inputs: InstallerPath, Installer
    // Outputs: HashPair
    void ReverifyInstallerHash(Execution::Context& context);

    // Verifies that the downloaded installer hash matches the hash in the manifest.
    // Required Args: None
    // Inputs: HashPair
    // Outputs: None
    void VerifyInstallerHash(Execution::Context& context);

    // Update Motw of the downloaded installer if applicable
    // Required Args: None
    // Inputs: HashPair, InstallerPath?, SourceId?
    // Outputs: None
    void UpdateInstallerFileMotwIfApplicable(Execution::Context& context);

    // This method appends appropriate extension to the downloaded installer.
    // ShellExecute uses file extension to launch the installer appropriately.
    // Required Args: None
    // Inputs: Installer, InstallerPath
    // Modifies: InstallerPath
    // Outputs: None
    void RenameDownloadedInstaller(Execution::Context& context);

    // Deletes the installer file.
    // Required Args: None
    // Inputs: InstallerPath
    // Outputs: None
    void RemoveInstaller(Execution::Context& context);

    // Sets the target download directory location if applicable.
    // Required Args: None
    // Inputs: Manifest
    // Outputs: None
    void SetDownloadDirectory(Execution::Context& context);

    // Exports the manifest yaml file for the downloaded package installer. Only applies to the 'winget download' command.
    // Required Args: None
    // Inputs: Manifest, Installer, DownloadDirectory
    // Outputs: None
    void ExportManifest(Execution::Context& context);

    // This method ensures requirements of download for later offline installation.
    // Required Args: None
    // Inputs: Installer
    // Outputs: None
    void EnsureSupportForDownload(Execution::Context& context);

    // This method initializes an empty InstallerDownloadAuthenticators map.
    // InstallerDownloadAuthenticators map is for reusing authenticators when downloading multiple installers.
    // Required Args: None
    // Inputs: None
    // Outputs: New empty InstallerDownloadAuthenticators
    void InitializeInstallerDownloadAuthenticatorsMap(Execution::Context& context);
}
