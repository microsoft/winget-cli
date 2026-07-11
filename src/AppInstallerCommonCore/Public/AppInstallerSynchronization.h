// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLanguageUtilities.h>
#include <AppInstallerProgress.h>
#include <wil/resource.h>

#include <chrono>
#include <functional>
#include <string_view>
#include <vector>

using namespace std::chrono_literals;


namespace AppInstaller::Synchronization
{
    // This is a standard named mutex.
    // It must be acquired and released (or destroyed) on the same thread, just as all Windows mutexes must be.
    struct CrossProcessLock
    {
        CrossProcessLock(std::string_view name);
        CrossProcessLock(const std::wstring& name);

        ~CrossProcessLock();

        CrossProcessLock(const CrossProcessLock&) = delete;
        CrossProcessLock& operator=(const CrossProcessLock&) = delete;

        CrossProcessLock(CrossProcessLock&&) = default;
        CrossProcessLock& operator=(CrossProcessLock&&) = default;

        // Acquires the lock; cancellation is enabled via the progress object.
        // Returns true when the lock is acquired and false if the wait is cancelled.
        bool Acquire(IProgressCallback& progress);

        // Optionally release the lock before destroying the object.
        void Release();

        // Attempts to acquire the mutex without a wait.
        // Returns true if it was able, false if not.
        bool TryAcquireNoWait();

        // Indicates whether the lock is held.
        operator bool() const;

    private:
        wil::unique_mutex m_mutex;
        wil::mutex_release_scope_exit m_lock;
        DWORD m_lockThreadId = 0;
    };

    // This lock is used to prevent multiple winget related processes from attempting to install (or uninstall) at the same time.
    // It must be acquired and released (or destroyed) on the same thread, just as all Windows mutexes must be.
    struct CrossProcessInstallLock : public CrossProcessLock
    {
        CrossProcessInstallLock() : CrossProcessLock(L"WinGetCrossProcessInstallLock") {}
    };
}
