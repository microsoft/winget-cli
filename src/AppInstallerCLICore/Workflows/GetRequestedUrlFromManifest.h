#pragma once
#include <AppInstallerProgress.h>
#include "ExecutionContext.h"

#include <filesystem>
#include <optional>

namespace AppInstaller::CLI::Workflow
{
    // Pulls the Urls from the Manifest
    // Required Args: None
    // Inputs: Context
    // Outputs: None
    void GetRequestedUrlFromManifest(Execution::Context& context);

}