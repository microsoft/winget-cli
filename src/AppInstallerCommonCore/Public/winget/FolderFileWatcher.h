// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <unordered_set>
#pragma warning( push )
#pragma warning ( disable : 6387 28196 )
#include <wil/filesystem.h>
#pragma warning( pop )

namespace AppInstaller::Utility
{
    // Watch for new/renamed files recursively in a given directory with an optional extension.
    struct FolderFileWatcher
    {
        FolderFileWatcher(const std::filesystem::path& path, const std::optional<std::string>& ext = std::nullopt);
        ~FolderFileWatcher() {};

        FolderFileWatcher(const FolderFileWatcher&) = delete;
        FolderFileWatcher& operator=(const FolderFileWatcher&) = delete;

        FolderFileWatcher(FolderFileWatcher&&) = default;
        FolderFileWatcher& operator=(FolderFileWatcher&&) = default;

        void Start();
        void Stop();

        const std::unordered_set<std::filesystem::path>& Files() { return m_files; }
        const std::filesystem::path& FolderPath() { return m_path; }

    private:
        std::filesystem::path m_path;
        std::optional<std::string> m_ext;
        std::unordered_set<std::filesystem::path> m_files;
        wil::unique_folder_change_reader m_changeReader;
    };
}