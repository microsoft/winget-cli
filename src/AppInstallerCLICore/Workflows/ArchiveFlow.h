// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Scans the archive file if downloaded from a local manifest
    // Required Args: None
    // Inputs: InstallerPath
    // Outputs: None
    void ScanArchiveFromLocalManifest(Execution::Context& context);

    // Extracts the files from an archive
    // Required Args: None
    // Inputs: InstallerPath
    // Outputs: None
    void ExtractFilesFromArchive(Execution::Context& context);

    // Verifies that the NestedInstaller exists and sets the InstallerPath
    // Required Args: None
    // Inputs: Installer, InstallerPath
    // Outputs: None
    void VerifyAndSetNestedInstaller(Execution::Context& context);

    // Verifies that the metadata related to the NestedInstaller is valid
    // Required Args: None
    // Inputs: Installer, InstallerPath
    // Outputs: None
    void EnsureValidNestedInstallerMetadataForArchiveInstall(Execution::Context& context);
}