// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerRuntime.h"
#include "CheckpointManager.h"
#include "Microsoft/CheckpointIndex.h"
#include "ResumeFlow.h"

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
        std::string resumeGuidString { context.Args.GetArg(Execution::Args::Type::ResumeId) };
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

        // Find a good place to check this.
        checkpointManager.Initialize(checkpointId);

        const auto& resumeStateClientVersion = checkpointManager.GetClientVersion();
        if (AppInstaller::Runtime::GetClientVersion().get() != resumeStateClientVersion)
        {
            context.Reporter.Error() << Resource::String::ClientVersionMismatchError(Utility::LocIndView{ resumeStateClientVersion }) << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_CLIENT_VERSION_MISMATCH);
        }

        if (!checkpointManager.HasContext())
        {
            context.Reporter.Error() << Resource::String::ResumeStateDataNotFoundError << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INVALID_RESUME_STATE);
        }
    }

    void Checkpoint::operator()(Execution::Context& context) const
    {
        if (!Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Resume))
        {
            return;
        }

        if (WI_IsFlagClear(context.GetFlags(), Execution::ContextFlag::Resume))
        {
            if (context.GetCheckpoint() == m_checkpointName)
            {
                // We have reached the checkpoint, begin enabling workflows.
                context.DisableWorkflowExecution(false);
            }
        }
        else
        {
            // If it is not a resume, simply capture the data in the index.
            context.Checkpoint(m_checkpointName, m_contextData);
        }
    }
}
