// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include <AppInstallerSynchronization.h>
#include <AppInstallerStrings.h>


namespace AppInstaller::Synchronization
{
    using namespace std::string_view_literals;

    constexpr std::wstring_view s_CrossProcessReaderWriteLock_MutexSuffix = L".mutex"sv;
    constexpr std::wstring_view s_CrossProcessReaderWriteLock_SemaphoreSuffix = L".sem"sv;

    // Arbitrary limit that should not ever cause a problem (theoretically 1 per process)
    constexpr LONG s_CrossProcessReaderWriteLock_MaxReaders = 16;

    CrossProcessReaderWriteLock::~CrossProcessReaderWriteLock()
    {
        for (LONG i = 0; i < m_semaphoreReleases; ++i)
        {
            m_semaphore.ReleaseSemaphore();
        }
    }

    CrossProcessReaderWriteLock CrossProcessReaderWriteLock::LockForRead(std::string_view name)
    {
        CrossProcessReaderWriteLock result(name);

        DWORD status = 0;
        auto lock = result.m_mutex.acquire(&status);
        THROW_HR_IF(E_UNEXPECTED, status != WAIT_OBJECT_0);

        // We are taking ownership of releasing this in the destructor
        status = ::WaitForSingleObjectEx(result.m_semaphore.get(), INFINITE, FALSE);
        THROW_HR_IF(E_UNEXPECTED, status != WAIT_OBJECT_0);

        result.m_semaphoreReleases = 1;
        return result;
    }

    CrossProcessReaderWriteLock CrossProcessReaderWriteLock::LockForWrite(std::string_view name)
    {
        CrossProcessReaderWriteLock result(name);

        DWORD status = 0;
        auto lock = result.m_mutex.acquire(&status);
        THROW_HR_IF(E_UNEXPECTED, status != WAIT_OBJECT_0);

        for (LONG i = 0; i < s_CrossProcessReaderWriteLock_MaxReaders; ++i)
        {
            // We are taking ownership of releasing these in the destructor
            status = ::WaitForSingleObjectEx(result.m_semaphore.get(), INFINITE, FALSE);
            THROW_HR_IF(E_UNEXPECTED, status != WAIT_OBJECT_0);
            result.m_semaphoreReleases = i + 1;
        }

        return result;
    }

    CrossProcessReaderWriteLock::CrossProcessReaderWriteLock(std::string_view name)
    {
        THROW_HR_IF(E_INVALIDARG, name.find('\\') != std::string::npos);

        std::wstring mutexName = Utility::ConvertToUTF16(name);
        std::wstring semName = mutexName;

        mutexName += s_CrossProcessReaderWriteLock_MutexSuffix;
        semName += s_CrossProcessReaderWriteLock_SemaphoreSuffix;

        m_mutex.create(mutexName.c_str(), 0, SYNCHRONIZE);
        m_semaphore.create(s_CrossProcessReaderWriteLock_MaxReaders, s_CrossProcessReaderWriteLock_MaxReaders, semName.c_str(), SYNCHRONIZE | SEMAPHORE_MODIFY_STATE);
    }
}
