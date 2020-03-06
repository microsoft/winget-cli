// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLanguageUtilities.h>
#include <wil/resource.h>

#include <string_view>


namespace AppInstaller::Synchronization
{
    // A fairly simple cross process (same session) reader-writer lock.
    // The primary purpose is for sources to control access to their backing stores.
    // Due to this design goal, these limitations exist:
    // - Starves readers when a writer comes in.
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

        static CrossProcessReaderWriteLock LockForRead(std::string_view name);

        static CrossProcessReaderWriteLock LockForWrite(std::string_view name);

    private:
        CrossProcessReaderWriteLock(std::string_view name);

        wil::unique_mutex m_mutex;
        wil::unique_semaphore m_semaphore;
        ResetWhenMovedFrom<LONG> m_semaphoreReleases{ 0 };
    };
}
