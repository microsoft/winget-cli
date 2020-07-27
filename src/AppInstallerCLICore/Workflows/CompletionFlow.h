// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"
#include "WorkflowBase.h"
#include <AppInstallerRepositorySearch.h>

namespace AppInstaller::CLI::Workflow
{
    // Outputs completion possibilities for the source name argument.
    // Required Args: None
    // Inputs: CompletionData
    // Outputs: None
    void CompleteSourceName(Execution::Context& context);

    // Terminates the context if the completion word is empty.
    // Required Args: None
    // Inputs: CompletionData
    // Outputs: None
    void RequireCompletionWordNonEmpty(Execution::Context& context);

    // Ensures the the file exists and is not a directory.
    // Required Args: the one given
    // Inputs: None
    // Outputs: None
    struct CompleteWithSearchResultField : public WorkflowTask
    {
        CompleteWithSearchResultField(Repository::ApplicationMatchField field) : WorkflowTask("CompleteWithSearchResultField"), m_field(field) {}

        void operator()(Execution::Context& context) const override;

    private:
        Repository::ApplicationMatchField m_field;
    };
}
