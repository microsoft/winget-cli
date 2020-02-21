// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"

#include <AppInstallerSynchronization.h>

using namespace AppInstaller::Synchronization;

TEST_CASE("CPRWL_MultipleReaders", "[CrossProcessReaderWriteLock]")
{
    std::string name = "AppInstCPRWLTests";

    wil::unique_event signal;
    signal.create();

    CrossProcessReaderWriteLock mainThreadLock = CrossProcessReaderWriteLock::LockForRead(name);

    std::thread otherThread([&name, &signal]() {
            CrossProcessReaderWriteLock otherThreadLock = CrossProcessReaderWriteLock::LockForRead(name);
            signal.SetEvent();
        });
    // In the event of bugs, we don't want to block the test waiting forever
    otherThread.detach();

    // Wait up to a second for the other thread to do one thing...
    REQUIRE(signal.wait(1000));
}

TEST_CASE("CPRWL_WriterBlocksReader", "[CrossProcessReaderWriteLock]")
{
    std::string name = "AppInstCPRWLTests";

    wil::unique_event signal;
    signal.create();

    {
        CrossProcessReaderWriteLock mainThreadLock = CrossProcessReaderWriteLock::LockForWrite(name);

        std::thread otherThread([&name, &signal]() {
            CrossProcessReaderWriteLock otherThreadLock = CrossProcessReaderWriteLock::LockForRead(name);
            signal.SetEvent();
            });
        // In the event of bugs, we don't want to block the test waiting forever
        otherThread.detach();

        REQUIRE(!signal.wait(1000));
    }

    // Upon release of the writer, the other thread should signal
    REQUIRE(signal.wait(1000));
}

TEST_CASE("CPRWL_ReaderBlocksWriter", "[CrossProcessReaderWriteLock]")
{
    std::string name = "AppInstCPRWLTests";

    wil::unique_event signal;
    signal.create();

    {
        CrossProcessReaderWriteLock mainThreadLock = CrossProcessReaderWriteLock::LockForRead(name);

        std::thread otherThread([&name, &signal]() {
            CrossProcessReaderWriteLock otherThreadLock = CrossProcessReaderWriteLock::LockForWrite(name);
            signal.SetEvent();
            });
        // In the event of bugs, we don't want to block the test waiting forever
        otherThread.detach();

        REQUIRE(!signal.wait(1000));
    }

    // Upon release of the writer, the other thread should signal
    REQUIRE(signal.wait(1000));
}

TEST_CASE("CPRWL_WriterBlocksWriter", "[CrossProcessReaderWriteLock]")
{
    std::string name = "AppInstCPRWLTests";

    wil::unique_event signal;
    signal.create();

    {
        CrossProcessReaderWriteLock mainThreadLock = CrossProcessReaderWriteLock::LockForWrite(name);

        std::thread otherThread([&name, &signal]() {
            CrossProcessReaderWriteLock otherThreadLock = CrossProcessReaderWriteLock::LockForWrite(name);
            signal.SetEvent();
            });
        // In the event of bugs, we don't want to block the test waiting forever
        otherThread.detach();

        REQUIRE(!signal.wait(1000));
    }

    // Upon release of the writer, the other thread should signal
    REQUIRE(signal.wait(1000));
}
