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
        // Relative file path to the startup menu folder.
        // Same installers write the links to user startup folder or machine startup folder depending on the scope
        // the installers were running for. So only relative file paths were collected. And seems enough.
        AppInstaller::Manifest::string_t RelativeFilePath;
        // Heuristic startup link type.
        AppInstaller::Manifest::InstalledFileTypeEnum FileType = AppInstaller::Manifest::InstalledFileTypeEnum::Unknown;
    };

    struct InstallationMetadata
    {
        // Installed files metadata. Currently only capturing files pointed by a startup link.
        AppInstaller::Manifest::InstallationMetadataInfo InstalledFiles;
        // Startup links metadata.
        std::vector<InstalledStartupLinkFile> StartupLinkFiles;
    };

    struct InstalledFilesCorrelation
    {
        // Constructor initializes the file watchers.
        InstalledFilesCorrelation();
        virtual ~InstalledFilesCorrelation() = default;

        // Start the file watcher before the package installation.
        virtual void StartFileWatcher();

        // Stop the file watcher after the package installation.
        virtual void StopFileWatcher();

        // Correlates the given manifest against the data previously collected with capture calls.
        virtual InstallationMetadata CorrelateForNewlyInstalled(
            const Manifest::Manifest& manifest,
            const std::string& arpInstallLocation);

    private:
        struct FileWatcherFiles
        {
            // FileWatcher folder base.
            std::filesystem::path Folder;
            // List of files represented as relative file path to the base folder.
            std::vector<std::filesystem::path> Files;
        };

        std::vector<AppInstaller::Utility::FolderFileWatcher> m_fileWatchers;
        std::vector<FileWatcherFiles> m_files;
    };
}
