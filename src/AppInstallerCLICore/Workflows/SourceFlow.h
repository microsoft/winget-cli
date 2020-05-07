// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Gets the current source list.
    // Required Args: None
    // Inputs: None
    // Outputs: SourceList
    void GetSourceList(Execution::Context& context);

    // Gets the source list, filtering it if SourceName is present.
    // Required Args: None
    // Inputs: None
    // Outputs: SourceList
    void GetSourceListWithFilter(Execution::Context& context);

    // Checks the source list against the inputs to ensure a successful add after this.
    // Required Args: SourceName, SourceArg
    // Inputs: SourceList
    // Outputs: None
    void CheckSourceListAgainstAdd(Execution::Context& context);

    // Adds the source.
    // Required Args: SourceName, SourceArg
    // Inputs: None
    // Outputs: None
    void AddSource(Execution::Context& context);

    // Lists the sources in SourceList.
    // Required Args: None
    // Inputs: SourceList
    // Outputs: None
    void ListSources(Execution::Context& context);

    // Updates the sources in SourceList.
    // Required Args: None
    // Inputs: SourceList
    // Outputs: None
    void UpdateSources(Execution::Context& context);

    // Removes the sources in SourceList.
    // Required Args: None
    // Inputs: SourceList
    // Outputs: None
    void RemoveSources(Execution::Context& context);

    // Asks the user if they are ok with dropping the sources in SourceList.
    // Required Args: None
    // Inputs: SourceList
    // Outputs: None
    void QueryUserForSourceReset(Execution::Context& context);

    // Drops the sources in SourceList.
    // Required Args: None
    // Inputs: SourceList
    // Outputs: None
    void ResetSourceList(Execution::Context& context);

    // Drops all sources.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void ResetAllSources(Execution::Context& context);
}
