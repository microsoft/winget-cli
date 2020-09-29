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

    // Outputs the matched field for the results.
    // Required Args: None
    // Inputs: SearchResult
    // Outputs: None
    void CompleteWithMatchedField(Execution::Context& context);

    // Outputs the versions available for the single search result.
    // Required Args: None
    // Inputs: CompletionData, SearchResult
    // Outputs: None
    void CompleteWithSearchResultVersions(Execution::Context& context);

    // Outputs the channels available for the single search result.
    // Required Args: None
    // Inputs: CompletionData, SearchResult
    // Outputs: None
    void CompleteWithSearchResultChannels(Execution::Context& context);

    // Executes the appropriate completion flow for the given argument in the context of a command
    // that targets a single manifest (ex. show or install).
    // Required Args: None
    // Inputs: CompletionData
    // Outputs: None
    struct CompleteWithSingleSemanticsForValue : public WorkflowTask
    {
        CompleteWithSingleSemanticsForValue(Execution::Args::Type type) : WorkflowTask("CompleteWithSingleSemanticsForValue"), m_type(type) {}

        void operator()(Execution::Context& context) const override;

    private:
        Execution::Args::Type m_type;
    };

    // Executes the appropriate completion flow for the given argument in the context of a command
    // that targets a single manifest (ex. show or install), using the already open source.
    // Required Args: None
    // Inputs: CompletionData, Source
    // Outputs: None
    struct CompleteWithSingleSemanticsForValueUsingExistingSource : public WorkflowTask
    {
        CompleteWithSingleSemanticsForValueUsingExistingSource(Execution::Args::Type type) : WorkflowTask("CompleteWithSingleSemanticsForValueUsingExistingSource"), m_type(type) {}

        void operator()(Execution::Context& context) const override;

    private:
        Execution::Args::Type m_type;
    };

    // Outputs an empty line to indicate that there are no completions.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void CompleteWithEmptySet(Execution::Context& context);
}
