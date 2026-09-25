// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <optional>
#include <string>
#include <vector>

namespace AppInstaller::CLI::Execution::StructuredOutput
{
    enum class Mode
    {
        Installed,
        AvailableUpgrades,
    };

    struct Package
    {
        std::string Name;
        std::string Id;
        std::string InstalledVersion;
        std::vector<std::string> AvailableVersions;
        bool IsUpdateAvailable = false;
        std::optional<std::string> Source;
        std::optional<std::string> UpgradeVersion;
    };

    struct PackageResult
    {
        std::vector<Package> Packages;
        bool Truncated = false;
    };
}
