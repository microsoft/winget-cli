// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Selects the package versions to list on the exported file
    // Required Args: None
    // Inputs: SearchResult
    // Outputs: PackageCollection
    void SelectVersionsToExport(Execution::Context& context);

    // Exports a collection of packages to a JSON import file
    // Required Args: OutputFile
    // Inputs: PackageCollection
    // Outputs: None
    void WriteImportFile(Execution::Context& context);

    // Reads the contents of an import file
    // Required Args: ImportFile
    // Inputs: None
    // Outputs: PackageCollection
    void ReadImportFile(Execution::Context& context);

    // Opens the sources specified in an import file
    // Required Args: None
    // Inputs: PackageCollection
    // Outputs: Sources
    void OpenSourcesForImport(Execution::Context& context);

    // Create the search requests and install sub-contexts for all the imported packages.
    // Needs the sources for all packages and the installed source
    // Required Args: None
    // Inputs: PackageCollection, Sources, Source
    // Outputs: PackageSubContexts
    //   SubContext Inputs: None
    //   SubContext Outputs: Source, SearchRequest
    void GetSearchRequestsForImport(Execution::Context& context);

    // Installs all the packages found in the import file.
    // Required Args: None
    // Inputs: PackageSubContexts
    // Outputs: None
    void InstallImportedPackages(Execution::Context& context);
}
