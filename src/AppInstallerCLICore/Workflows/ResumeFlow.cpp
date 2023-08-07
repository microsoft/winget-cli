// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerRuntime.h"
#include "ResumeFlow.h"
#include "CheckpointManager.h"
#include "Microsoft/CheckpointIndex.h"
#include "Commands/RootCommand.h"

using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::CLI::Checkpoint;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Utility::literals;

namespace AppInstaller::CLI::Workflow
{
    void EnsureSupportForResume(Execution::Context& context)
    {
        std::string resumeGuidString { context.Args.GetArg(Execution::Args::Type::ResumeGuid) };
        if (!Utility::IsValidGuidString(resumeGuidString))
        {
            context.Reporter.Error() << Resource::String::InvalidResumeGuidError(Utility::LocIndView{ resumeGuidString }) << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INVALID_RESUME_GUID);
        }

        GUID checkpointId = Utility::ConvertToGuid(resumeGuidString);

        if (!std::filesystem::exists(AppInstaller::Repository::Microsoft::CheckpointIndex::GetCheckpointIndexPath(checkpointId)))
        {
            context.Reporter.Error() << Resource::String::ResumeGuidNotFoundError(Utility::LocIndView{ resumeGuidString }) << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_RESUME_GUID_NOT_FOUND);
        }

        CheckpointManager::Instance().Initialize(checkpointId);

        const auto& resumeStateClientVersion = CheckpointManager::Instance().GetClientVersion();
        if (AppInstaller::Runtime::GetClientVersion().get() != resumeStateClientVersion)
        {
            context.Reporter.Error() << Resource::String::ClientVersionMismatchError(Utility::LocIndView{ resumeStateClientVersion }) << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_CLIENTVERSION_MISMATCH);
        }

        if (!CheckpointManager::Instance().HasContext())
        {
            context.Reporter.Error() << Resource::String::ResumeStateDataNotFoundError << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INVALID_RESUME_STATE);
        }
    }
}
