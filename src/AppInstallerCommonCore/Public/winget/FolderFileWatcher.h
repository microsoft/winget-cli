// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"

namespace AppInstaller::Utility
{
    // Watch for new/renamed files recursively in a given directory.
    struct FolderFileWatcher
    {
        FolderFileWatcher(const std::filesystem::path& path);
        ~FolderFileWatcher();

        FolderFileWatcher(const FolderFileWatcher&) = delete;
        FolderFileWatcher& operator=(const FolderFileWatcher&) = delete;

        FolderFileWatcher(FolderFileWatcher&&) = delete;
        FolderFileWatcher& operator=(FolderFileWatcher&&) = delete;

        void start();
        void stop();

        const std::unordered_set<std::string>& files() { return m_files; }

    private:
        std::filesystem::path m_path;
        std::unordered_set<std::string> m_files;
        wil::unique_folder_change_reader m_changeReader;
    };

    // Watch for new/renamed files recursively for a given extension in a given directory.
    struct FolderFileExtensionWatcher
    {
        FolderFileExtensionWatcher(const std::filesystem::path& path, const std::string& ext);
        ~FolderFileExtensionWatcher();

        FolderFileExtensionWatcher(const FolderFileExtensionWatcher&) = delete;
        FolderFileExtensionWatcher& operator=(const FolderFileExtensionWatcher&) = delete;

        FolderFileExtensionWatcher(FolderFileExtensionWatcher&&) = delete;
        FolderFileExtensionWatcher& operator=(FolderFileExtensionWatcher&&) = delete;

        void start();
        void stop();

        const std::unordered_set<std::string>& files() { return m_files; }

    private:
        std::filesystem::path m_path;
        std::string m_ext;
        std::unordered_set<std::string> m_files;
        wil::unique_folder_change_reader m_changeReader;
    };
}