// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    enum UriValidationSource
    {
        ConfigurationSource,
        PackageCatalogSource,
    };

    // Composite flow that chooses what to do based on whether or not the
    // configuration flow is being run.
    // Required Args: None
    // Inputs: IsConfigurationFlow
    // Outputs: None
    struct ExecuteUriValidation: public WorkflowTask
    {
        ExecuteUriValidation(UriValidationSource uriValidationSource) : WorkflowTask("ExecuteUriValidation"), m_uriValidationSource(uriValidationSource) {}

        void operator()(Execution::Context& context) const override;

    private:
        UriValidationSource m_uriValidationSource;
    };
}

