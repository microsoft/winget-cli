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

    // Performs searches on each of the subcontexts with the semantics of targeting a single package for each one.
    // Required Args: bool indicating whether the flow is for upgrade
    // Inputs: PackageSubContexts
    // Outputs: None
    //   SubContext Inputs: Source, SearchRequest
    //   SubContext Outputs: SearchResult
    struct SearchSubContextsForSingle : public WorkflowTask
    {
        SearchSubContextsForSingle(bool isUpgrade = false) : WorkflowTask("SearchSubContextsForSingle"), m_isUpgrade(isUpgrade) {}

        void operator()(Execution::Context& context) const override;

    private:
        bool m_isUpgrade;
    };
}