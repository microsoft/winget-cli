// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"

#include <AppInstallerSynchronization.h>

using namespace AppInstaller::Synchronization;

TEST_CASE("CPIL_BlocksOthers", "[CrossProcessInstallLock]")
{
    wil::unique_event signal;
    signal.create();
    AppInstaller::ProgressCallback progress;

    {
        CrossProcessInstallLock mainThreadLock;
        mainThreadLock.Acquire(progress);

        std::thread otherThread([&signal, &progress]() {
            CrossProcessInstallLock otherThreadLock;
            otherThreadLock.Acquire(progress);
            signal.SetEvent();
            });
        // In the event of bugs, we don't want to block the test waiting forever
        otherThread.detach();

        REQUIRE(!signal.wait(500));
    }

    // Upon release of the writer, the other thread should signal
    REQUIRE(signal.wait(500));
}

TEST_CASE("CPIL_CancelEndsWait", "[CrossProcessInstallLock]")
{
    wil::unique_event signal;
    signal.create();
    AppInstaller::ProgressCallback progress;

    CrossProcessInstallLock mainThreadLock;
    mainThreadLock.Acquire(progress);

    std::optional<bool> otherThreadAcquireResult = std::nullopt;

    std::thread otherThread([&]() {
        CrossProcessInstallLock otherThreadLock;
        otherThreadAcquireResult = otherThreadLock.Acquire(progress);
        signal.SetEvent();
        });
    // In the event of bugs, we don't want to block the test waiting forever
    otherThread.detach();

    REQUIRE(!signal.wait(500));

    progress.Cancel();

    // Upon release of the writer, the other thread should signal
    REQUIRE(signal.wait(500));

    REQUIRE(otherThreadAcquireResult.has_value());
    REQUIRE(!otherThreadAcquireResult.value());
}
