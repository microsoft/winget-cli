// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Ensures that the arguments provided supports a resume.
    // Required Args: None
    // Inputs: ResumeGuid
    // Outputs: None
    void EnsureSupportForResume(Execution::Context& context);

    // Applies a checkpoint to the context workflow.
    struct Checkpoint : public WorkflowTask
    {
        Checkpoint(std::string_view checkpointName, std::vector<Execution::Data> contextData) :
            WorkflowTask("ApplyCheckpoint"),
            m_checkpointName(checkpointName),
            m_contextData(contextData) {}

        void operator()(Execution::Context& context) const override;

    private:
        std::string_view m_checkpointName;
        std::vector<Execution::Data> m_contextData;
    };
}
