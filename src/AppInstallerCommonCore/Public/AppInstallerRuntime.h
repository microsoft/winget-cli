// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <filesystem>

namespace AppInstaller::Runtime
{
    // Determines whether the process is running in a packaged context or not.
    bool IsRunningInPackagedContext();

    // Gets the path to the temp location.
    std::filesystem::path GetPathToTemp();
}
