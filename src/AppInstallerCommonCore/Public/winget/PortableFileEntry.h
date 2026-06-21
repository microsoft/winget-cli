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
        Symlink,
        Hardlink
    };

    // Metadata representation of a portable file placed down during installation
    struct PortableFileEntry
    {
        // Version 1.0
        PortableFileType FileType = PortableFileType::Unknown;
        std::string SHA256;
        std::string SymlinkTarget;
        std::filesystem::path CurrentPath;

        void SetFilePath(const std::filesystem::path& path)
        {
            if (FileType != PortableFileType::Symlink)
            {
                m_filePath = std::filesystem::weakly_canonical(path);
            }
            else
            {
                m_filePath = path;
            }
        };

        std::filesystem::path GetFilePath() const { return m_filePath; };

        static PortableFileEntry CreateFileEntry(const std::filesystem::path& currentPath, const std::filesystem::path& targetPath, const std::string& sha256)
        {
            PortableFileEntry fileEntry;
            fileEntry.FileType = PortableFileType::File;
            fileEntry.CurrentPath = currentPath;
            fileEntry.SetFilePath(targetPath);

            if (sha256.empty())
            {
                fileEntry.SHA256 = Utility::SHA256::ConvertToString(Utility::SHA256::ComputeHashFromFile(currentPath));
            }
            else
            {
                fileEntry.SHA256 = sha256;
            }
            return fileEntry;
        }

        static PortableFileEntry CreateSymlinkEntry(const std::filesystem::path& symlinkPath, const std::filesystem::path& targetPath)
        {
            PortableFileEntry symlinkEntry;
            symlinkEntry.FileType = PortableFileType::Symlink;
            symlinkEntry.SetFilePath(symlinkPath);
            symlinkEntry.SymlinkTarget = targetPath.u8string();
            return symlinkEntry;
        }

        static PortableFileEntry CreateHardlinkEntry(const std::filesystem::path& hardlinkPath, const std::filesystem::path& targetPath, const std::string& sha256 = {})
        {
            PortableFileEntry hardlinkEntry;
            hardlinkEntry.FileType = PortableFileType::Hardlink;
            hardlinkEntry.CurrentPath = targetPath;
            hardlinkEntry.SetFilePath(hardlinkPath);
            // Use provided SHA256 or empty (will be computed from target after install)
            hardlinkEntry.SHA256 = sha256;
            return hardlinkEntry;
        }

        static PortableFileEntry CreateDirectoryEntry(const std::filesystem::path& currentPath, const std::filesystem::path& directoryPath)
        {
            PortableFileEntry directoryEntry;
            directoryEntry.FileType = PortableFileType::Directory;
            directoryEntry.CurrentPath = currentPath;
            directoryEntry.SetFilePath(directoryPath);
            return directoryEntry;
        }

    private:
        std::filesystem::path m_filePath;
    };
}