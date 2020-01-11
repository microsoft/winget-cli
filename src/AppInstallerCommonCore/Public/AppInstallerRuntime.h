// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <filesystem>

namespace AppInstaller::Runtime
{
    enum class Architecture
    {
        unknown = -1,
        neutral,
        x86,
        x64,
        arm,
        arm64
    };

    // Determines whether the process is running in a packaged context or not.
    bool IsRunningInPackagedContext();

    // Gets the path to the temp location.
    std::filesystem::path GetPathToTemp();

    Architecture GetSystemArchitecture();

    std::set<Architecture> GetApplicableArchitectures();

    bool IsApplicableArchitecture(Architecture arch);
}
