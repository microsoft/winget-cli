// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Composite flow that chooses what to do based on whether or not the
    // configuration flow is being run.
    // Required Args: None
    // Inputs: IsConfigurationFlow
    // Outputs: None
    struct ExecuteUriValidation: public WorkflowTask
    {
        ExecuteUriValidation(bool isConfigurationFlow) : WorkflowTask("ExecuteUriValidation"), m_isConfigurationFlow(isConfigurationFlow) {}

        void operator()(Execution::Context& context) const override;

    private:
        bool m_isConfigurationFlow;
    };
}

