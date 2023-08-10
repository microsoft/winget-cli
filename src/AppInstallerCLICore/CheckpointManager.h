// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

#include "Microsoft/CheckpointIndex.h"
#include <guiddef.h>

namespace AppInstaller::Repository::Microsoft
{
    struct CheckpointIndex;
}

namespace AppInstaller::CLI::Checkpoint
{
    struct CheckpointManager
    {
        CheckpointManager(GUID id = {});
        ~CheckpointManager();

        std::string GetClientVersion();

        std::string GetCommandName(int contextId);

        std::string GetArguments();

        void RecordMetadata(std::string_view checkpointName, std::string_view commandName, std::string_view commandLineString, std::string clientVersion);

        template<class T>
        void RecordContextData(std::string_view checkpointName, T data) {};

    private:
        GUID m_checkpointId = {};
        std::shared_ptr<AppInstaller::Repository::Microsoft::CheckpointIndex> m_checkpointIndex = nullptr;

        void CleanUpIndex();
    };
}
