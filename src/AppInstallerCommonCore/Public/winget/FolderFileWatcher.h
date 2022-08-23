// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"

namespace AppInstaller::Utility
{
    // Watch for new/renamed files recursively in a given directory with an optional extension.
    struct FolderFileWatcher
    {
        FolderFileWatcher(const std::filesystem::path& path, const std::optional<std::string>& ext = std::nullopt);
        ~FolderFileWatcher() {};

        FolderFileWatcher(const FolderFileWatcher&) = delete;
        FolderFileWatcher& operator=(const FolderFileWatcher&) = delete;

        FolderFileWatcher(FolderFileWatcher&&) = delete;
        FolderFileWatcher& operator=(FolderFileWatcher&&) = delete;

        void Start();
        void Stop();

        const std::unordered_set<std::filesystem::path>& files() { return m_files; }

    private:
        std::filesystem::path m_path;
        std::optional<std::string> m_ext;
        std::unordered_set<std::filesystem::path> m_files;
        wil::unique_folder_change_reader m_changeReader;
    };
}