// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <filesystem>
#include "AppInstallerArchitecture.h"

namespace AppInstaller::Runtime
{
    // Determines whether the process is running in a packaged context or not.
    bool IsRunningInPackagedContext();

    // Gets the path to the temp location.
    std::filesystem::path GetPathToTemp();

    AppInstaller::Utility::Architecture GetSystemArchitecture();
}
