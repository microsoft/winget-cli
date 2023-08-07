// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

#include <guiddef.h>

namespace AppInstaller::Repository::Microsoft
{
    struct CheckpointIndex;
}

namespace AppInstaller::CLI::Checkpoint
{
    struct CheckpointManager
    {
        CheckpointManager(const CheckpointManager&) = delete;
        CheckpointManager& operator=(const CheckpointManager&) = delete;
        CheckpointManager(CheckpointManager&&) = delete;
        CheckpointManager& operator=(CheckpointManager&&) = delete;

        static CheckpointManager& Instance()
        {
            static CheckpointManager checkpointManager;
            return checkpointManager;
        }

        void Initialize(GUID checkpointId = {});

        void Checkpoint(Execution::Context& context, Execution::CheckpointFlag flag);

        bool HasContext();

        void AddContext(int contextId);

        void RemoveContext(int contextId);

        std::string GetClientVersion();

        std::string GetCommandName(int contextId);

        int GetFirstContextId();

        Execution::CheckpointFlag GetLastCheckpoint(int contextId);

#ifndef AICLI_DISABLE_TEST_HOOKS
        // Only used by unit tests to release any test indexes for proper cleanup.
        void ManualReset()
        {
            m_checkpointId = GUID_NULL;
            m_checkpointIndex.reset();
        }
#endif


    private:
        CheckpointManager() = default;
        ~CheckpointManager();
        GUID m_checkpointId = {};
        std::shared_ptr<AppInstaller::Repository::Microsoft::CheckpointIndex> m_checkpointIndex = nullptr;

        void CleanUpIndex();

        void SaveCheckpoint(Execution::Context& context, Execution::CheckpointFlag flag);
        void LoadCheckpoint(Execution::Context& context, Execution::CheckpointFlag flag);

        void PopulateContextArgsFromIndex(Execution::Context& context);
        void RecordContextArgsToIndex(Execution::Context& context);
    };
}
