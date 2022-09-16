// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "AppInstallerSHA256.h"
#include <string>
#include <filesystem>

namespace AppInstaller::Portable
{
    // File type enum of the portable file
    enum class PortableFileType
    {
        Unknown,
        File,
        Directory,
        Symlink
    };

    // Metadata representation of a portable file placed down during installation
    struct PortableFileEntry
    {
        // Version 1.0
        PortableFileType FileType = PortableFileType::Unknown;
        std::string SHA256;
        std::string SymlinkTarget;
        std::filesystem::path CurrentPath;

        void SetFilePath(const std::filesystem::path& path) { m_filePath = std::filesystem::weakly_canonical(path); };

        std::filesystem::path GetFilePath() const { return m_filePath; };

        static PortableFileEntry CreateFileEntry(const std::filesystem::path& currentPath, const std::filesystem::path& targetPath)
        {
            PortableFileEntry fileEntry;
            fileEntry.CurrentPath = currentPath;
            fileEntry.SetFilePath(targetPath);
            fileEntry.FileType = PortableFileType::File;
            fileEntry.SHA256 = Utility::SHA256::ConvertToString(Utility::SHA256::ComputeHashFromFile(currentPath));
            return fileEntry;
        }

        static PortableFileEntry CreateSymlinkEntry(const std::filesystem::path& symlinkPath, const std::filesystem::path& targetPath)
        {
            PortableFileEntry symlinkEntry;
            symlinkEntry.SetFilePath(symlinkPath);
            symlinkEntry.SymlinkTarget = targetPath.u8string();
            symlinkEntry.FileType = PortableFileType::Symlink;
            return symlinkEntry;
        }

        static PortableFileEntry CreateDirectoryEntry(const std::filesystem::path& directoryPath)
        {
            PortableFileEntry directoryEntry;
            directoryEntry.SetFilePath(directoryPath);
            directoryEntry.FileType = PortableFileType::Directory;
            return directoryEntry;
        }

    private:
        std::filesystem::path m_filePath;
    };
}