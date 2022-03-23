// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "ExecutionContext.h"
#include <filesystem>

// PortableInstallHelper provides functionality specific to installing portable executables.
namespace AppInstaller::CLI::Workflow
{
    // Gets the install location
    std::filesystem::path GetPortableInstallLocation(Manifest::ScopeEnum scope, Utility::Architecture arch);

    // Writes to uninstall registry
    void WriteToAppPathsRegistry(std::string_view entryName, const std::filesystem::path& exePath, bool enablePath);

    // Writes to AppPaths registry
    bool WriteToUninstallRegistry(Manifest::ScopeEnum scope, std::string& packageIdentifier, Manifest::AppsAndFeaturesEntry& entry);

    void PortableInstallImpl(Execution::Context& context);
}