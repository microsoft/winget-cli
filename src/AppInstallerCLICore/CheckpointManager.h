// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <guiddef.h>
#include "winget/ManifestCommon.h"

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

        void LoadContextData(std::string_view checkpointName, AppInstaller::Manifest::ManifestInstaller& installer);

        void RecordContextData(std::string_view checkpointName, const AppInstaller::Manifest::ManifestInstaller& installer);

    private:
        GUID m_checkpointId = {};
        std::shared_ptr<AppInstaller::Repository::Microsoft::CheckpointIndex> m_checkpointIndex = nullptr;

        void CleanUpIndex();
    };
}
