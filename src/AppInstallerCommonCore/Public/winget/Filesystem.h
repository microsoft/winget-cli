// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"

namespace AppInstaller::Filesystem
{
    // Checks if the file system at path supports named streams/ADS
    bool SupportsNamedStreams(const std::filesystem::path& path);

    // Checks if the file system at path supports hard links
    bool SupportsHardLinks(const std::filesystem::path& path);

    // Checks if the file system at path support reparse points
    bool SupportsReparsePoints(const std::filesystem::path& path);

    // Checks that the path is absolute and does not contain any dot-dot elements or symbolic links.
    bool IsPathCanonical(const std::filesystem::path& path);

    // Renames the file to a new path.
    void RenameFile(const std::filesystem::path& from, const std::filesystem::path& to);
}