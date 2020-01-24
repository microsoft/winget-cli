// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "AppInstallerArchitecture.h"

#include <filesystem>
#include <string>

namespace AppInstaller::Runtime
{
    // Determines whether the process is running in a packaged context or not.
    bool IsRunningInPackagedContext();

    // Determines the current version of the client and returns it.
    std::string GetClientVersion();

    // Gets the path to the temp location.
    std::filesystem::path GetPathToTemp();

    // Gets the system's architecture as Architecture enum
    AppInstaller::Utility::Architecture GetSystemArchitecture();
}
