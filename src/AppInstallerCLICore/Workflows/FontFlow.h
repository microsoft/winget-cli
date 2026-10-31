// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Reports the installed fonts as a table.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void ReportInstalledFonts(Execution::Context& context);

    // Checks elevation support before font installation consumes installed state.
    // Required Args: None
    // Inputs: Installer
    // Outputs: None
    void EnsureSupportForFontInstall(Execution::Context& context);

    // Installs the font package.
    // Required Args: None
    // Inputs: Manifest, Installer, InstallerPath
    // Outputs: None
    void FontInstallImpl(Execution::Context& context);

    // Uninstalls the font package.
    // Checks both the selected package scope and the effective target scope before using installed data.
    // Required Args: None
    // Inputs: InstalledPackageVersion, Manifest?
    // Outputs: None
    void FontUninstallImpl(Execution::Context& context);
}
