// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    using namespace std::string_view_literals;

    // Token specified in installer args will be replaced by proper value.
    static constexpr std::string_view ARG_TOKEN_LOGPATH = "<LOGPATH>"sv;
    static constexpr std::string_view ARG_TOKEN_INSTALLPATH = "<INSTALLPATH>"sv;

    // Ensures that there is an applicable installer.
    // Required Args: None
    // Inputs: Installer
    // Outputs: None
    void EnsureApplicableInstaller(Execution::Context& context);

    // Shows the installation disclaimer.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void ShowInstallationDisclaimer(Execution::Context& context);

    // Composite flow that chooses what to do based on the installer type.
    // Required Args: None
    // Inputs: Manifest, Installer
    // Outputs: None
    void DownloadInstaller(Execution::Context& context);

    // Downloads the file referenced by the Installer.
    // Required Args: None
    // Inputs: Installer
    // Outputs: HashPair, InstallerPath
    void DownloadInstallerFile(Execution::Context& context);

    // Computes the hash of the MSIX signature file.
    // Required Args: None
    // Inputs: Installer
    // Outputs: HashPair
    void GetMsixSignatureHash(Execution::Context& context);

    // Gets the source list, filtering it if SourceName is present.
    // Required Args: None
    // Inputs: HashPair
    // Outputs: SourceList
    void VerifyInstallerHash(Execution::Context& context);

    // Update Motw of the downloaded installer if applicable
    // Required Args: None
    // Inputs: HashPair, InstallerPath?, SourceId?
    // Outputs: None
    void UpdateInstallerFileMotwIfApplicable(Execution::Context& context);

    // Composite flow that chooses what to do based on the installer type.
    // Required Args: None
    // Inputs: Installer, InstallerPath
    // Outputs: None
    void ExecuteInstaller(Execution::Context& context);

    // Runs the installer via ShellExecute.
    // Required Args: None
    // Inputs: Installer, InstallerPath
    // Outputs: None
    void ShellExecuteInstall(Execution::Context& context);

    // Deploys the MSIX.
    // Required Args: None
    // Inputs: Manifest?, Installer || InstallerPath
    // Outputs: None
    void MsixInstall(Execution::Context& context);

    // Deletes the installer file.
    // Required Args: None
    // Inputs: InstallerPath
    // Outputs: None
    void RemoveInstaller(Execution::Context& context);

    // Installs a specific package installer.
    // Required Args: None
    // Inputs: Manifest, Installer
    // Outputs: None
    void InstallPackageInstaller(Execution::Context& context);

    // Installs a specific package version.
    // Required Args: None
    // Inputs: Manifest, PackageVersion, Source
    // Outputs: None
    void InstallPackageVersion(Execution::Context& context);

    // Installs multiple packages.
    // Required Args: None
    // Inputs: Manifests
    // Outputs: None
    void InstallMultiple(Execution::Context& context);

    // Stores the existing set of packages in ARP.
    // Required Args: None
    // Inputs: Installer
    // Outputs: ARPSnapshot
    void SnapshotARPEntries(Execution::Context& context);

    // Reports on the changes between the stored ARPSnapshot and the current values.
    // Required Args: None
    // Inputs: ARPSnapshot?, Manifest, PackageVersion
    // Outputs: None
    void ReportARPChanges(Execution::Context& context);
}
