// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLanguageUtilities.h>
#include <AppInstallerProgress.h>
#include <wil/resource.h>

#include <chrono>
#include <string_view>
#include <vector>

using namespace std::chrono_literals;


namespace AppInstaller::Synchronization
{
    // A fairly simple cross process (same session) reader-writer lock.
    // The primary purpose is for sources to control access to their backing stores.
    // Due to this design goal, these limitations exist:
    // - Starves new readers when a writer comes in.
    // - Readers are limited to an arbitrarily chosen limit.
    // - Not re-entrant (although repeated read locking will work, it will consume additional slots).
    // - No upgrade from reader to writer.
    struct CrossProcessReaderWriteLock
    {
        // Create unheld lock.
        CrossProcessReaderWriteLock() = default;

        ~CrossProcessReaderWriteLock();

        CrossProcessReaderWriteLock(const CrossProcessReaderWriteLock&) = delete;
        CrossProcessReaderWriteLock& operator=(const CrossProcessReaderWriteLock&) = delete;

        CrossProcessReaderWriteLock(CrossProcessReaderWriteLock&&) = default;
        CrossProcessReaderWriteLock& operator=(CrossProcessReaderWriteLock&&) = default;

        static CrossProcessReaderWriteLock LockShared(std::string_view name);
        static CrossProcessReaderWriteLock LockShared(std::string_view name, IProgressCallback& progress);

        static CrossProcessReaderWriteLock LockExclusive(std::string_view name);
        static CrossProcessReaderWriteLock LockExclusive(std::string_view name, IProgressCallback& progress);
        static CrossProcessReaderWriteLock LockExclusive(std::string_view name, std::chrono::milliseconds timeout);

        operator bool() const;

    private:
        static CrossProcessReaderWriteLock Lock(bool shared, std::string_view name, std::chrono::milliseconds timeout, IProgressCallback* progress);

        std::vector<wil::unique_mutex> m_mutexesHeld;
    };
}
