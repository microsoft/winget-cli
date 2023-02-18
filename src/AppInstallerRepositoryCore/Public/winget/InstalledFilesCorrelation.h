// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/Manifest.h>
#include <winget/FolderFileWatcher.h>
#include <optional>

namespace AppInstaller::Repository::Correlation
{
    // TODO: This definition could be moved to Manifest when winget supports launch scenarios.
    struct InstalledStartupLinkFile
    {
        AppInstaller::Manifest::string_t RelativeFilePath;
        AppInstaller::Manifest::InstalledFileTypeEnum FileType = AppInstaller::Manifest::InstalledFileTypeEnum::Other;
    };

    struct InstallationMetadata
    {
        AppInstaller::Manifest::InstallationMetadataInfo InstalledFiles;
        std::vector<InstalledStartupLinkFile> StartupLinkFiles;
    };

    struct InstalledFilesCorrelation
    {
        // Constructor initializes the file watchers.
        InstalledFilesCorrelation();

        // Start the file watcher before the package installation.
        void StartFileWatcher();

        // Stop the file watcher after the package installation.
        void StopFileWatcher();

        // Correlates the given manifest against the data previously collected with capture calls.
        InstallationMetadata CorrelateForNewlyInstalled(
            const Manifest::Manifest& manifest,
            const std::string& arpInstallLocation);

    private:
        struct FileWatcherFiles
        {
            std::filesystem::path Folder;
            std::vector<std::filesystem::path> Files;
        };

        std::vector<AppInstaller::Utility::FolderFileWatcher> m_fileWatchers;
        std::vector<FileWatcherFiles> m_files;
    };
}
