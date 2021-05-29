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

    // Arbitrary limit that should not ever cause a problem (theoretically 1 per process)
    constexpr size_t s_CrossProcessReaderWriteLock_MaxReaders = 8;

    namespace
    {
        void VerifyName(std::string_view name)
        {
            THROW_HR_IF(E_INVALIDARG, name.find('\\') != std::string::npos);
        }

        void VerifyInfiniteWaitStatus(DWORD status)
        {
            THROW_LAST_ERROR_IF(status == WAIT_FAILED);
            THROW_HR_IF(E_UNEXPECTED, status == WAIT_TIMEOUT);
        }

        wil::unique_mutex OpenControlMutex(const std::wstring& name)
        {
            std::wstring mutexName = name;
            mutexName += s_CrossProcessReaderWriteLock_MutexSuffix;

            wil::unique_mutex result;
            result.create(mutexName.c_str(), 0, SYNCHRONIZE);
            return result;
        }

        wil::unique_mutex OpenAccessMutex(const std::wstring& name, size_t index)
        {
            THROW_HR_IF(E_INVALIDARG, index >= s_CrossProcessReaderWriteLock_MaxReaders);
            std::wostringstream strstr;
            strstr << name << L'.' << index;

            wil::unique_mutex result;
            result.create(strstr.str().c_str(), 0, SYNCHRONIZE);
            return result;
        }
    }

    CrossProcessReaderWriteLock::~CrossProcessReaderWriteLock()
    {
        for (auto& mutex : m_mutexesHeld)
        {
            ReleaseMutex(mutex.get());
        }
    }

    CrossProcessReaderWriteLock CrossProcessReaderWriteLock::LockShared(std::string_view name)
    {
        VerifyName(name);
        CrossProcessReaderWriteLock result;

        std::wstring wideName = Utility::ConvertToUTF16(name);

        DWORD status = 0;
        wil::unique_mutex controlMutex = OpenControlMutex(wideName);
        auto lock = controlMutex.acquire(&status);
        VerifyInfiniteWaitStatus(status);

        // Acquire the first mutex we can find that is open, or wait on all of them.
        // Use the process id as an arbitrary value in an attempt to reduce collisions
        // while still allowing for re-entrance to not be arbitrary.
        size_t offset = GetProcessId(GetCurrentProcess()) % s_CrossProcessReaderWriteLock_MaxReaders;
        std::vector<wil::unique_mutex> allReaders;
        HANDLE readerHandles[s_CrossProcessReaderWriteLock_MaxReaders]{};

        for (size_t i = 0; i < s_CrossProcessReaderWriteLock_MaxReaders; ++i)
        {
            size_t index = (i + offset) % s_CrossProcessReaderWriteLock_MaxReaders;

            wil::unique_mutex current = OpenAccessMutex(wideName, index);
            status = ::WaitForSingleObjectEx(current.get(), 0, FALSE);

            if (status == WAIT_OBJECT_0 || status == WAIT_ABANDONED)
            {
                // We found an empty one, continue on with it
                result.m_mutexesHeld.emplace_back(std::move(current));
                return result;
            }
            else if (status == WAIT_TIMEOUT)
            {
                readerHandles[i] = current.get();
                allReaders.emplace_back(std::move(current));
            }
            else
            {
                THROW_LAST_ERROR();
            }
        }

        // We did not find an open mutex above, so wait on them all.
        status = WaitForMultipleObjectsEx(s_CrossProcessReaderWriteLock_MaxReaders, readerHandles, FALSE, INFINITE, FALSE);
        VerifyInfiniteWaitStatus(status);

        size_t acquiredIndex = 0;
        if (status >= WAIT_OBJECT_0 && status < (WAIT_OBJECT_0 + s_CrossProcessReaderWriteLock_MaxReaders))
        {
            acquiredIndex = status - WAIT_OBJECT_0;
        }
        else if (status >= WAIT_ABANDONED_0 && status < (WAIT_ABANDONED_0 + s_CrossProcessReaderWriteLock_MaxReaders))
        {
            acquiredIndex = status - WAIT_ABANDONED_0;
        }
        else
        {
            THROW_HR(E_UNEXPECTED);
        }

        // Take the one that was acquired
        result.m_mutexesHeld.emplace_back(std::move(allReaders[acquiredIndex]));
        return result;
    }

    CrossProcessReaderWriteLock CrossProcessReaderWriteLock::LockExclusive(std::string_view name)
    {
        VerifyName(name);
        CrossProcessReaderWriteLock result;

        std::wstring wideName = Utility::ConvertToUTF16(name);

        DWORD status = 0;
        wil::unique_mutex controlMutex = OpenControlMutex(wideName);
        auto lock = controlMutex.acquire(&status);
        VerifyInfiniteWaitStatus(status);

        // Open and wait on all of the access mutexes.
        std::vector<wil::unique_mutex> allReaders;
        HANDLE readerHandles[s_CrossProcessReaderWriteLock_MaxReaders]{};

        for (size_t i = 0; i < s_CrossProcessReaderWriteLock_MaxReaders; ++i)
        {
            wil::unique_mutex current = OpenAccessMutex(wideName, i);
            readerHandles[i] = current.get();
            allReaders.emplace_back(std::move(current));
        }

        status = WaitForMultipleObjectsEx(s_CrossProcessReaderWriteLock_MaxReaders, readerHandles, TRUE, INFINITE, FALSE);
        VerifyInfiniteWaitStatus(status);

        // All mutexes were acquired
        result.m_mutexesHeld = std::move(allReaders);
        return result;
    }

    CrossProcessReaderWriteLock CrossProcessReaderWriteLock::LockExclusive(std::string_view name, std::chrono::milliseconds timeout)
    {
        THROW_HR_IF(E_INVALIDARG, timeout.count() > INFINITE);

        VerifyName(name);
        CrossProcessReaderWriteLock result;

        std::wstring wideName = Utility::ConvertToUTF16(name);
        auto start = std::chrono::steady_clock::now();

        DWORD status = 0;
        wil::unique_mutex controlMutex = OpenControlMutex(wideName);
        auto lock = controlMutex.acquire(&status, static_cast<DWORD>(timeout.count()));
        THROW_LAST_ERROR_IF(status == WAIT_FAILED);

        if (status == WAIT_TIMEOUT)
        {
            return result;
        }

        // Open and wait on all of the access mutexes.
        std::vector<wil::unique_mutex> allReaders;
        HANDLE readerHandles[s_CrossProcessReaderWriteLock_MaxReaders]{};

        for (size_t i = 0; i < s_CrossProcessReaderWriteLock_MaxReaders; ++i)
        {
            wil::unique_mutex current = OpenAccessMutex(wideName, i);
            readerHandles[i] = current.get();
            allReaders.emplace_back(std::move(current));
        }

        DWORD millisToWait = static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(timeout - (std::chrono::steady_clock::now() - start)).count());
        status = WaitForMultipleObjectsEx(s_CrossProcessReaderWriteLock_MaxReaders, readerHandles, TRUE, millisToWait, FALSE);
        THROW_LAST_ERROR_IF(status == WAIT_FAILED);

        if (status == WAIT_TIMEOUT)
        {
            return result;
        }

        // All mutexes were acquired
        result.m_mutexesHeld = std::move(allReaders);
        return result;
    }

    CrossProcessReaderWriteLock::operator bool() const
    {
        return !m_mutexesHeld.empty();
    }
}
