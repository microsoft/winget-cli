// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <guiddef.h>

namespace AppInstaller::Repository::Microsoft
{
    struct CheckpointIndex;
}

namespace AppInstaller::CLI::Checkpoint
{
    struct CheckpointManager
    {
        // Constructor for an existing resume id.
        CheckpointManager(GUID id);
        // Constructor for a new resume save state.
        CheckpointManager(std::string_view commandName, std::string_view commandArguments, std::string_view clientVersion);
        
        ~CheckpointManager();

        std::string GetClientVersion();

        std::string GetCommandName();

        std::string GetArguments();

        template<class T>
        void RecordContextData(std::string_view checkpointName, T data)
        {
            UNREFERENCED_PARAMETER(checkpointName);
            UNREFERENCED_PARAMETER(data);
        }

    private:
        GUID m_checkpointId = {};
        std::shared_ptr<AppInstaller::Repository::Microsoft::CheckpointIndex> m_checkpointIndex = nullptr;

        void CleanUpIndex();
    };
}
