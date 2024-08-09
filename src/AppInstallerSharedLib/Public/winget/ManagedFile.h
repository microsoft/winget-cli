// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <wil/resource.h>
#include <filesystem>

namespace AppInstaller::Utility
{
    // Struct that holds a file handle and may perform additional operations on exit
    struct ManagedFile
    {
        ManagedFile() = default;

        ManagedFile(const ManagedFile&) = delete;
        ManagedFile& operator=(const ManagedFile&) = delete;

        ManagedFile(ManagedFile&&) = default;
        ManagedFile& operator=(ManagedFile&&) = default;

        HANDLE GetFileHandle() const { return m_fileHandle.get(); }
        const std::filesystem::path& GetFilePath() const { return m_filePath; }

        // Always creates a new write locked file at the path given. desiredAccess is passed to CreateFile call.
        static ManagedFile CreateWriteLockedFile(const std::filesystem::path& path, DWORD desiredAccess, bool deleteOnExit);

        // Always opens an existing file at the path given with write locked. desiredAccess is passed to CreateFile call.
        static ManagedFile OpenWriteLockedFile(const std::filesystem::path& path, DWORD desiredAccess);

        ~ManagedFile();

    private:
        std::filesystem::path m_filePath;
        wil::unique_handle m_fileHandle;
        bool m_deleteFileOnExit = false;
    };
}