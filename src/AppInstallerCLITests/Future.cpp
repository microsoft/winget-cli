// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerFuture.h>

using namespace AppInstaller;

struct FutureProgress : public IFutureProgress
{
    // IFutureProgress
    void OnStarted() override
    {
        if (Started)
        {
            Started();
        }
    }

    void OnProgress(uint64_t current, uint64_t maximum, FutureProgressType type) override
    {
        if (Progress)
        {
            Progress(current, maximum, type);
        }
    }

    void OnCompleted(bool cancelled) override
    {
        if (Completed)
        {
            Completed(cancelled);
        }
    }

    std::function<void()> Started;
    std::function<void(uint64_t, uint64_t, FutureProgressType)> Progress;
    std::function<void(bool)> Completed;
};

TEST_CASE("Future_Basic", "[Future]")
{
    int input = 42;

    Future<int> f{ std::packaged_task<int(IPromiseKeeperProgress*)>{ [input](IPromiseKeeperProgress*) { return input; } } };
    int result = f.Get().value();

    REQUIRE(input == result);
}

TEST_CASE("Future_Optional", "[Future]")
{
    int input = 42;

    Future<std::optional<int>> f{ std::packaged_task<std::optional<int>(IPromiseKeeperProgress*)>{ [input](IPromiseKeeperProgress*) { return input; } } };
    int result = f.Get().value();

    REQUIRE(input == result);
}

TEST_CASE("Future_Callbacks", "[Future]")
{
    int input = 42;
    uint64_t progress = 110;
    uint64_t maximum = 100;
    FutureProgressType type = FutureProgressType::Percent;

    bool startedCalled = false;
    uint64_t progressActual = 0;
    uint64_t maximumActual = 0;
    FutureProgressType typeActual = FutureProgressType::None;
    bool cancelledActual = false;

    FutureProgress futureProgress;
    futureProgress.Started = [&]() { startedCalled = true; };
    futureProgress.Progress = [&](uint64_t p, uint64_t m, FutureProgressType t) { progressActual = p; maximumActual = m; typeActual = t; };
    futureProgress.Completed = [&](bool c) { cancelledActual = c; };

    Future<std::optional<int>> f{ std::packaged_task<std::optional<int>(IPromiseKeeperProgress*)>{
        [&](IPromiseKeeperProgress* p) {
            p->OnProgress(progress, maximum, type);
            return input;
        }
    } };
    f.SetProgressReceiver(&futureProgress);
    int result = f.Get().value();

    REQUIRE(input == result);
    REQUIRE(startedCalled);
    REQUIRE(progress == progressActual);
    REQUIRE(maximum == maximumActual);
    REQUIRE(type == typeActual);
    REQUIRE(!cancelledActual);
}

TEST_CASE("Future_Cancelled", "[Future]")
{
    int input = 42;
    uint64_t progress = 110;
    uint64_t maximum = 100;
    FutureProgressType type = FutureProgressType::Percent;

    bool startedCalled = false;
    uint64_t progressActual = 0;
    uint64_t maximumActual = 0;
    FutureProgressType typeActual = FutureProgressType::None;
    bool cancelledActual = false;

    Future<std::optional<int>> f{ std::packaged_task<std::optional<int>(IPromiseKeeperProgress*)>{
        [&](IPromiseKeeperProgress* p) {
            p->OnProgress(progress, maximum, type);
            if (p->IsCancelled())
            {
                return 0;
            }
            else
            {
                return input;
            }
        }
    } };

    FutureProgress futureProgress;
    futureProgress.Started = [&]() { startedCalled = true; };
    futureProgress.Progress = [&](uint64_t p, uint64_t m, FutureProgressType t) {
        progressActual = p; maximumActual = m; typeActual = t;
        f.Cancel();
    };
    futureProgress.Completed = [&](bool c) { cancelledActual = c; };
    f.SetProgressReceiver(&futureProgress);

    auto result = f.Get();

    REQUIRE(!result.has_value());
    REQUIRE(startedCalled);
    REQUIRE(progress == progressActual);
    REQUIRE(maximum == maximumActual);
    REQUIRE(type == typeActual);
    REQUIRE(cancelledActual);
}
