// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/Manifest.h>
#include <winget/FolderFileWatcher.h>
#include <optional>

namespace AppInstaller::Repository::Correlation
{
    struct InstalledShellLinkFile
    {
        AppInstaller::Manifest::string_t RelativeFilePath;
        AppInstaller::Manifest::InstalledFileTypeEnum FileType = AppInstaller::Manifest::InstalledFileTypeEnum::Other;
    };

    struct InstallationMetadata
    {
        AppInstaller::Manifest::InstallationMetadataInfo InstalledFiles;
        std::vector<InstalledShellLinkFile> ShellLinkFiles;
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
        std::optional<InstallationMetadata> CorrelateForNewlyInstalled(
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
