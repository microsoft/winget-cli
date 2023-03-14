// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

// Workflow tasks related to dealing with multiple package queries at once.

namespace AppInstaller::CLI::Workflow
{
    // Gets the search requests for multiple queries from the command line.
    // Required Args: None
    // Inputs: Source
    // Outputs: PackageSubContexts
    //   SubContext Inputs: None
    //   SubContext Outputs: Source, SearchRequest
    void GetMultiSearchRequests(Execution::Context& context);

    // Performs searches on each of the sub-contexts with the semantics of targeting a single package for each one.
    // Required Args: a value indicating the purpose of the search
    // Inputs: PackageSubContexts
    // Outputs: None
    //   SubContext Inputs: Source, SearchRequest
    //   SubContext Outputs: SearchResult
    struct SearchSubContextsForSingle : public WorkflowTask
    {
        SearchSubContextsForSingle(OperationType operation = OperationType::Install) : WorkflowTask("SearchSubContextsForSingle"), m_operationType(operation) {}

        void operator()(Execution::Context& context) const override;

    private:
        OperationType m_operationType;
    };
}