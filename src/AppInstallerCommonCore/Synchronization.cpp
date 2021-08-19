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

    // A milliseconds version of INFINITE
    constexpr std::chrono::milliseconds s_CrossProcessReaderWriteLock_Infinite = static_cast<std::chrono::milliseconds>(INFINITE);

    // The amount of time that we wait in between checking for cancellation
    constexpr std::chrono::milliseconds s_CrossProcessReaderWriteLock_WaitLoopTime = 250ms;

    // Arbitrary limit that should not ever cause a problem (theoretically 1 per process)
    constexpr size_t s_CrossProcessReaderWriteLock_MaxReaders = 8;

    namespace
    {
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
        return Lock(true, name, s_CrossProcessReaderWriteLock_Infinite, nullptr);
    }

    CrossProcessReaderWriteLock CrossProcessReaderWriteLock::LockShared(std::string_view name, IProgressCallback& progress)
    {
        return Lock(true, name, s_CrossProcessReaderWriteLock_Infinite, &progress);
    }

    CrossProcessReaderWriteLock CrossProcessReaderWriteLock::LockExclusive(std::string_view name)
    {
        return Lock(false, name, s_CrossProcessReaderWriteLock_Infinite, nullptr);
    }

    CrossProcessReaderWriteLock CrossProcessReaderWriteLock::LockExclusive(std::string_view name, IProgressCallback& progress)
    {
        return Lock(false, name, s_CrossProcessReaderWriteLock_Infinite, &progress);
    }

    CrossProcessReaderWriteLock CrossProcessReaderWriteLock::LockExclusive(std::string_view name, std::chrono::milliseconds timeout)
    {
        return Lock(false, name, timeout, nullptr);
    }

    CrossProcessReaderWriteLock::operator bool() const
    {
        return !m_mutexesHeld.empty();
    }

    CrossProcessReaderWriteLock CrossProcessReaderWriteLock::Lock(
        bool shared,
        std::string_view name,
        std::chrono::milliseconds timeout,
        IProgressCallback* progress)
    {
        auto start = std::chrono::steady_clock::now();

        // Verify inputs
        THROW_HR_IF(E_INVALIDARG, name.find('\\') != std::string::npos);
        THROW_HR_IF(E_INVALIDARG, timeout.count() > INFINITE);

        CrossProcessReaderWriteLock result;
        std::wstring wideName = Utility::ConvertToUTF16(name);

        // Acquire overall control mutex
        DWORD status = 0;
        wil::unique_mutex controlMutex = OpenControlMutex(wideName);
        auto lock = controlMutex.acquire(&status, static_cast<DWORD>(timeout.count()));
        THROW_LAST_ERROR_IF(status == WAIT_FAILED);

        if (status == WAIT_TIMEOUT || (progress && progress->IsCancelled()))
        {
            return result;
        }

        // Open all needed access mutexes
        std::vector<wil::unique_mutex> allAccessMutexes;
        HANDLE waitHandles[s_CrossProcessReaderWriteLock_MaxReaders]{};

        if (shared)
        {
            // Acquire the first access mutex we can find that is open, or all of them if needed.
            // Use the process id as an arbitrary value in an attempt to reduce collisions
            // while still allowing for re-entrance to not be arbitrary.
            size_t offset = GetProcessId(GetCurrentProcess()) % s_CrossProcessReaderWriteLock_MaxReaders;

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
                    waitHandles[i] = current.get();
                    allAccessMutexes.emplace_back(std::move(current));
                }
                else
                {
                    THROW_LAST_ERROR();
                }
            }
        }
        else
        {
            // Open all of the access mutexes.
            for (size_t i = 0; i < s_CrossProcessReaderWriteLock_MaxReaders; ++i)
            {
                wil::unique_mutex current = OpenAccessMutex(wideName, i);
                waitHandles[i] = current.get();
                allAccessMutexes.emplace_back(std::move(current));
            }
        }

        // Wait for one/all of the mutexes (or cancellation)
        bool waitAgain = true;
        while (waitAgain && (!progress || !progress->IsCancelled()))
        {
            DWORD millisecondsToWait = 0;
            if (progress)
            {
                if (timeout == s_CrossProcessReaderWriteLock_Infinite)
                {
                    millisecondsToWait = static_cast<DWORD>(s_CrossProcessReaderWriteLock_WaitLoopTime.count());
                }
                else
                {
                    auto currentDuration = std::chrono::steady_clock::now() - start;
                    if (currentDuration >= timeout)
                    {
                        // Allow an attempt to acquire with no wait
                        millisecondsToWait = 0;
                        waitAgain = false;
                    }
                    else
                    {
                        auto durationToWait = timeout - currentDuration;
                        if (durationToWait > s_CrossProcessReaderWriteLock_WaitLoopTime)
                        {
                            durationToWait = s_CrossProcessReaderWriteLock_WaitLoopTime;
                        }
                        else
                        {
                            waitAgain = false;
                        }
                        millisecondsToWait = static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(durationToWait).count());
                    }
                }
            }
            else
            {
                // If there is no progress, we will do the full wait this time
                waitAgain = false;

                if (timeout == s_CrossProcessReaderWriteLock_Infinite)
                {
                    millisecondsToWait = INFINITE;
                }
                else
                {
                    auto currentDuration = std::chrono::steady_clock::now() - start;
                    if (currentDuration >= timeout)
                    {
                        // Allow an attempt to acquire with no wait
                        millisecondsToWait = 0;
                    }
                    else
                    {
                        millisecondsToWait = static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(timeout - currentDuration).count());
                    }
                }
            }

            status = WaitForMultipleObjectsEx(s_CrossProcessReaderWriteLock_MaxReaders, waitHandles, (shared ? FALSE : TRUE), millisecondsToWait, FALSE);
            THROW_LAST_ERROR_IF(status == WAIT_FAILED);

            if (status != WAIT_TIMEOUT)
            {
                break;
            }
        }

        if (status == WAIT_TIMEOUT || (progress && progress->IsCancelled()))
        {
            return result;
        }

        if (shared)
        {
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
            result.m_mutexesHeld.emplace_back(std::move(allAccessMutexes[acquiredIndex]));
        }
        else
        {
            result.m_mutexesHeld = std::move(allAccessMutexes);
        }

        return result;
    }
}
