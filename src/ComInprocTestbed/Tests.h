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

// Forces COM to unload the module and checks for leaked resources.
std::pair<bool, std::optional<Snapshot>> UnloadAndCheckForLeaks(std::optional<Snapshot> previousSnapshot, bool expectUnload);
