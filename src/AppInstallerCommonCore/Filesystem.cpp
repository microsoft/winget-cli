// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::Filesystem
{
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
            FILE_ATTRIBUTE_NORMAL, /*dwFlagsAndAttributes*/
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

    bool IsNonReservedFilename(const std::filesystem::path& path)
    {
        std::string filename= path.filename().u8string();
        std::string extension = path.extension().u8string();
        Utility::FindAndReplace(filename, extension, "");

        auto lastChar = filename.back();
        if (lastChar == '.' || lastChar == ' ')
        {
            return false;
        }

        for (const auto& illegalName : {
            "."sv, "CON"sv, "PRN"sv, "AUX"sv, "NUL"sv, "COM0"sv, "COM1"sv, "COM2"sv, "COM3"sv, "COM4"sv, "COM5"sv, "COM6"sv, "COM7"sv, "COM8"sv, "COM9"sv, "LPT0"sv,
            "LPT1"sv, "LPT2"sv, "LPT3"sv, "LPT4"sv, "LPT5"sv, "LPT6"sv, "LPT7"sv, "LPT8"sv, "LPT9"sv })
        {
            if (Utility::CaseInsensitiveEquals(filename, illegalName))
            {
                return false;
            }
        }

        return true;
    }
}