// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/ManagedFile.h"
#include "AppInstallerLogging.h"

namespace AppInstaller::Utility
{
    ManagedFile ManagedFile::CreateWriteLockedFile(const std::filesystem::path& path, DWORD desiredAccess, bool deleteOnExit)
    {
        ManagedFile file;
        file.m_fileHandle.reset(CreateFileW(path.c_str(), desiredAccess, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
        THROW_LAST_ERROR_IF(!file.m_fileHandle);
        file.m_filePath = path;
        file.m_deleteFileOnExit = deleteOnExit;

        return file;
    }

    ManagedFile ManagedFile::OpenWriteLockedFile(const std::filesystem::path& path, DWORD desiredAccess)
    {
        ManagedFile file;
        file.m_fileHandle.reset(CreateFileW(path.c_str(), desiredAccess, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
        THROW_LAST_ERROR_IF(!file.m_fileHandle);
        file.m_filePath = path;

        return file;
    }

    ManagedFile::~ManagedFile()
    {
        if (m_deleteFileOnExit)
        {
            if (m_fileHandle)
            {
                m_fileHandle.reset();
            }

            try
            {
                std::filesystem::remove(m_filePath);
            }
            catch (...)
            {
                AICLI_LOG(Core, Info, << "Failed to remove managed file at: " << m_filePath);
            }
        }
    }
}