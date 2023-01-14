// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

// Workflow tasks related to installing multiple packages at once.

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
    // Required Args: None
    // Inputs: PackageSubContexts
    // Outputs: None
    //   SubContext Inputs: Source, SearchRequest
    //   SubContext Outputs: SearchResult
    void SearchSubContextsForSingle(Execution::Context& context);
}