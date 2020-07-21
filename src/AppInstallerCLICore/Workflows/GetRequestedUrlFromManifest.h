#pragma once
#include <AppInstallerProgress.h>
#include "ExecutionContext.h"

#include <filesystem>
#include <optional>

// ShellExecuteInstallerHandler handles installers run through ShellExecute.
// Exe, Wix, Nullsoft, Msi and Inno should be handled by this installer handler.
namespace AppInstaller::CLI::Workflow
{
    // Pulls the Urls from the Manifest
    // Required Args: None
    // Inputs: Context
    // Outputs: None
    void GetRequestedUrlFromManifest(Execution::Context& context);

}