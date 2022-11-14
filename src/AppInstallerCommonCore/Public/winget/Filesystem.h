// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <filesystem>

namespace AppInstaller::Filesystem
{
    // Checks if the file system at path supports named streams/ADS
    bool SupportsNamedStreams(const std::filesystem::path& path);

    // Checks if the file system at path supports hard links
    bool SupportsHardLinks(const std::filesystem::path& path);

    // Checks if the file system at path support reparse points
    bool SupportsReparsePoints(const std::filesystem::path& path);

    // Checks if the canonical form of the path points to a location outside of the provided base path.
    bool PathEscapesBaseDirectory(const std::filesystem::path& target, const std::filesystem::path& base);

    // Renames the file to a new path.
    void RenameFile(const std::filesystem::path& from, const std::filesystem::path& to);

    // Creates a symlink that points to the target path.
    bool CreateSymlink(const std::filesystem::path& target, const std::filesystem::path& link);

    // Verifies that a symlink points to the target path.
    bool VerifySymlink(const std::filesystem::path& symlink, const std::filesystem::path& target);

    // Appends the .exe extension to the path if not present.
    void AppendExtension(std::filesystem::path& value, const std::string& extension);

    // Checks if the path is a symlink and exists.
    bool SymlinkExists(const std::filesystem::path& symlinkPath);
    bool CreateSymlink(const std::filesystem::path& path, const std::filesystem::path& target);

    // Get expanded file system path.
    std::filesystem::path GetExpandedPath(const std::string& path);
}