// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

struct Snapshot
{
    Snapshot();

    size_t ThreadCount = 0;
    size_t ModuleCount = 0;
    PROCESS_MEMORY_COUNTERS_EX2 Memory{};
};

// Represents a test that will be performed.
struct ITest
{
    virtual ~ITest() = default;

    // Runs an iteration of the test.
    virtual bool RunIteration() = 0;

    // Performs the final test validation.
    virtual bool RunFinal() = 0;
};

// A test that unloads the COM module and looks for resources that were not released.
struct UnloadAndCheckForLeaks : public ITest
{
    UnloadAndCheckForLeaks(bool shouldUnload);

    bool RunIteration() override;

    bool RunFinal() override;

private:
    bool m_shouldUnload;
    Snapshot m_initialSnapshot;
    std::vector<std::pair<Snapshot, Snapshot>> m_iterationSnapshots;
};
