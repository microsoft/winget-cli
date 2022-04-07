// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerProgress.h"
#include "winget/Registry.h"

namespace AppInstaller::Filesystem
{
    namespace
    {
        DWORD CALLBACK CopyFileProgressCallback(
            LARGE_INTEGER TotalFileSize,
            LARGE_INTEGER TotalBytesTransferred,
            LARGE_INTEGER StreamSize,
            LARGE_INTEGER StreamBytesTransferred,
            DWORD dwStreamNumber,
            DWORD dwCallbackReason,
            HANDLE hSourceFile,
            HANDLE hDestinationFile,
            LPVOID lpData
        )
        {
            UNREFERENCED_PARAMETER(StreamSize);
            UNREFERENCED_PARAMETER(StreamBytesTransferred);
            UNREFERENCED_PARAMETER(hSourceFile);
            UNREFERENCED_PARAMETER(hDestinationFile);

            ProgressCallback callback = static_cast<ProgressCallback>((ProgressCallback*)lpData);

            if (callback.IsCancelled())
            {
                return PROGRESS_CANCEL;
            }

            if (dwCallbackReason == CALLBACK_STREAM_SWITCH || dwStreamNumber == 1)
            {
                callback.BeginProgress();
            }

            if (dwCallbackReason == CALLBACK_CHUNK_FINISHED)
            {
                callback.OnProgress(TotalBytesTransferred.QuadPart, TotalFileSize.QuadPart, AppInstaller::ProgressType::Percent);
            }

            if (TotalBytesTransferred.QuadPart == TotalFileSize.QuadPart)
            {
                callback.EndProgress(true);
            }

            return PROGRESS_CONTINUE;
        }
    }

    void CreateSymlink(const std::filesystem::path& target, const std::filesystem::path& link)
    {
        if (std::filesystem::exists(link))
        {
            std::filesystem::remove(link);
        }

        std::filesystem::create_symlink(target, link);
    }

    DWORD CopyFileWithProgressCallback(const std::filesystem::path& from, const std::filesystem::path& to, IProgressCallback& progress)
    {
        bool result = CopyFileExW(from.c_str(), to.c_str(), &AppInstaller::Filesystem::CopyFileProgressCallback, &progress, FALSE, 0);
        DWORD exitCode = 0;
        if (!result)
        {
            exitCode = GetLastError();
        }

        return exitCode;
    }
}