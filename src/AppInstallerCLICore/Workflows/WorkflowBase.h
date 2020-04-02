// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Creates the source object.
    // Required Args: None
    // Inputs: None
    // Outputs: Source
    void OpenSource(Execution::Context& context);

    // Performs a search on the source.
    // Required Args: None
    // Inputs: Source
    // Outputs: SearchResult
    void SearchSource(Execution::Context& context);

    // Outputs the search results.
    // Required Args: None
    // Inputs: SearchResult
    // Outputs: None
    void ReportSearchResult(Execution::Context& context);

    // Ensures that there is only one result in the search.
    // Required Args: None
    // Inputs: SearchResult
    // Outputs: None
    void EnsureMatchesFromSearchResult(Execution::Context& context);

    // Ensures that there is only one result in the search.
    // Required Args: None
    // Inputs: SearchResult
    // Outputs: None
    void EnsureOneMatchFromSearchResult(Execution::Context& context);

    // Gets the manifest from a search result.
    // Required Args: None
    // Inputs: SearchResult
    // Outputs: Manifest
    void GetManifestFromSearchResult(Execution::Context& context);

    // Ensures the the file exists and is not a directory.
    // Required Args: the one given
    // Inputs: None
    // Outputs: None
    struct VerifyFile
    {
        VerifyFile(Execution::Args::Type arg) : m_arg(arg) {}

        void operator()(Execution::Context& context);

    private:
        Execution::Args::Type m_arg;
    };

    // Opens the manifest file provided on the command line.
    // Required Args: Manifest
    // Inputs: None
    // Outputs: Manifest
    void GetManifestFromArg(Execution::Context& context);

    // Composite flow that produces a manifest; either from one given on the command line or by searching.
    // Required Args: None
    // Inputs: None
    // Outputs: Manifest
    void GetManifest(Execution::Context& context);

    // Selects the installer from the manifest, if one is applicable.
    // Required Args: None
    // Inputs: Manifest
    // Outputs: Installer
    void SelectInstaller(Execution::Context& context);
}