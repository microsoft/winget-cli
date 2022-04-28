// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::Filesystem
{
    using namespace std::chrono_literals;
    using namespace std::string_view_literals;

    DWORD GetVolumeInformationFlagsByHandle(HANDLE anyFileHandle)
    {
        DWORD flags = 0;
        wchar_t fileSystemName[MAX_PATH];
        THROW_LAST_ERROR_IF(!GetVolumeInformationByHandleW(
            anyFileHandle, /*hFile*/
            NULL, /*lpVolumeNameBuffer*/
            0, /*nVolumeNameSize*/
            NULL, /*lpVolumeSerialNumber*/
            NULL, /*lpMaximumComponentLength*/
            &flags, /*lpFileSystemFlags*/
            fileSystemName, /*lpFileSystemNameBuffer*/
            MAX_PATH /*nFileSystemNameSize*/));

        // Vista and older does not report all flags, fix them up here
        if (!(flags & FILE_SUPPORTS_HARD_LINKS) && !_wcsicmp(fileSystemName, L"NTFS"))
        {
            flags |= FILE_SUPPORTS_HARD_LINKS | FILE_SUPPORTS_EXTENDED_ATTRIBUTES | FILE_SUPPORTS_OPEN_BY_FILE_ID | FILE_SUPPORTS_USN_JOURNAL;
        }

        return flags;
    }

    DWORD GetVolumeInformationFlags(const std::filesystem::path& anyPath)
    {
        wil::unique_hfile fileHandle{ CreateFileW(
            anyPath.c_str(), /*lpFileName*/
            0, /*dwDesiredAccess*/
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, /*dwShareMode*/
            NULL, /*lpSecurityAttributes*/
            OPEN_EXISTING, /*dwCreationDisposition*/
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, /*dwFlagsAndAttributes*/
            NULL /*hTemplateFile*/) };

        THROW_LAST_ERROR_IF(fileHandle.get() == INVALID_HANDLE_VALUE);

        return GetVolumeInformationFlagsByHandle(fileHandle.get());
    }

    bool SupportsNamedStreams(const std::filesystem::path& path)
    {
        return (GetVolumeInformationFlags(path) & FILE_NAMED_STREAMS) != 0;
    }

    bool SupportsHardLinks(const std::filesystem::path& path)
    {
        return (GetVolumeInformationFlags(path) & FILE_SUPPORTS_HARD_LINKS) != 0;
    }

    bool SupportsReparsePoints(const std::filesystem::path& path)
    {
        return (GetVolumeInformationFlags(path) & FILE_SUPPORTS_REPARSE_POINTS) != 0;
    }

    // Complicated rename algorithm due to somewhat arbitrary failures.
    // 1. First, try to rename.
    // 2. Then, create an empty file for the target, and attempt to rename.
    // 3. Then, try repeatedly for 500ms in case it is a timing thing.
    // 4. Attempt to use a hard link if available.
    // 5. Copy the file if nothing else has worked so far.
    void RenameFile(const std::filesystem::path& from, const std::filesystem::path& to)
    {
        // 1. First, try to rename.
        try
        {
            // std::filesystem::rename() handles motw correctly if applicable.
            std::filesystem::rename(from, to);
            return;
        }
        CATCH_LOG();

        // 2. Then, create an empty file for the target, and attempt to rename.
        //    This seems to fix things in certain cases, so we do it.
        try
        {
            {
                std::ofstream targetFile{ to };
            }
            std::filesystem::rename(from, to);
            return;
        }
        CATCH_LOG();

        // 3. Then, try repeatedly for 500ms in case it is a timing thing.
        for (int i = 0; i < 5; ++i)
        {
            try
            {
                std::this_thread::sleep_for(100ms);
                std::filesystem::rename(from, to);
                return;
            }
            CATCH_LOG();
        }

        // 4. Attempt to use a hard link if available.
        if (SupportsHardLinks(from))
        {
            try
            {
                // Create a hard link to the file; the installer will be left in the temp directory afterward
                // but it is better to succeed the operation and leave a file around than to fail.
                // First we have to remove the target file as the function will not overwrite.
                std::filesystem::remove(to);
                std::filesystem::create_hard_link(from, to);
                return;
            }
            CATCH_LOG();
        }

        // 5. Copy the file if nothing else has worked so far.
        // Create a copy of the file; the installer will be left in the temp directory afterward
        // but it is better to succeed the operation and leave a file around than to fail.
        std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
    }
}