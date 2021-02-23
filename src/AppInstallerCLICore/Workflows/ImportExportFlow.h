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

    // Finds the package versions to install matching their descriptions
    // Needs the sources for all packages and the installed source
    // Required Args: None
    // Inputs: PackageCollection, Sources, Source
    // Outputs: PackagesToInstall
    void SearchPackagesForImport(Execution::Context& context);
}
