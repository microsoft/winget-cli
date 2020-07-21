#pragma once
#include <AppInstallerProgress.h>
#include "ExecutionContext.h"

#include <filesystem>
#include <optional>

// ShellExecuteInstallerHandler handles installers run through ShellExecute.
// Exe, Wix, Nullsoft, Msi and Inno should be handled by this installer handler.
namespace AppInstaller::CLI::Workflow
{
    // Install is done through invoking ShellExecute on downloaded installer.
    // Required Args: None
    // Inputs: Manifest?, InstallerPath, InstallerArgs
    // Outputs: None
    void OpenUrlInDefaultBrowser(Execution::Context& context);

}