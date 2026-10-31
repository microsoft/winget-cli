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

    // Installs the font package.
    // Required Args: None
    // Inputs: Manifest, Scope, Rename, Location
    // Outputs: None
    void FontInstallImpl(Execution::Context& context);

    // Repairs the installed font package's files and registrations.
    // Required Args: None
    // Inputs: InstalledPackageVersion
    // Outputs: OperationReturnCode
    void FontRepairImpl(Execution::Context& context);

    // Uninstalls the font package.
    // Required Args: None
    // Inputs: Manifest, Scope, Rename, Location
    // Outputs: None
    void FontUninstallImpl(Execution::Context& context);
}
