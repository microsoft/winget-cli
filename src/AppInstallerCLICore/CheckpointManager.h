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

        void Initialize();

        void InitializeFromGuid(GUID checkpointId);

        void AddContext(int contextId);

        void RemoveContext(int contextId);

        void Checkpoint(Execution::Context& context, Execution::CheckpointFlags flag);

        std::unique_ptr<Execution::Context> CreateContextFromCheckpointIndex();

        std::string GetClientVersion();

        std::string GetCommandName(int contextId);

        int GetFirstContextId();

    private:
        CheckpointManager() = default;
        ~CheckpointManager();
        GUID m_checkpointId = {};
        std::shared_ptr<AppInstaller::Repository::Microsoft::CheckpointIndex> m_checkpointIndex = nullptr;

        bool CleanUpIndex();

        void SaveCheckpoint(Execution::Context& context, Execution::CheckpointFlags flag);
        void LoadCheckpoint(Execution::Context& context, Execution::CheckpointFlags flag);

        void PopulateContextArgsFromIndex(Execution::Context& context);
        void RecordContextArgsToIndex(Execution::Context& context);
    };
}
// 