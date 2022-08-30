// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/Manifest.h>
#include <winget/FolderFileWatcher.h>
#include <optional>

namespace AppInstaller::Repository::Correlation
{
    struct InstalledFilesCorrelation
    {
        // Constructor initializes the file watchers.
        InstalledFilesCorrelation();

        // Start the file watcher before the package installation.
        void StartFileWatcher();

        // Stop the file watcher after the package installation.
        void StopFileWatcher();

        // Correlates the given manifest against the data previously collected with capture calls.
        std::optional<AppInstaller::Manifest::InstallationMetadataInfo> CorrelateForNewlyInstalled(
            const Manifest::Manifest& manifest,
            const std::string& arpInstallLocation,
            const std::string& arpUninstallString);

    private:
        std::vector<AppInstaller::Utility::FolderFileWatcher> m_fileWatchers;
        std::vector<std::filesystem::path> m_files;
    };
}
