// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <filesystem>
#include "AppInstallerArchitecture.h"
#include "AppInstallerCLICore.h"

namespace AppInstaller::Runtime
{
    struct RuntimeException : public wil::ResultException
    {
        RuntimeException() : wil::ResultException(CLICORE_ERROR_RUNTIME_ERROR) {}
    };

    // Determines whether the process is running in a packaged context or not.
    bool IsRunningInPackagedContext();

    // Gets the path to the temp location.
    std::filesystem::path GetPathToTemp();

    // Gets the system's architecture as Architecture enum
    AppInstaller::Utility::Architecture GetSystemArchitecture();
}
