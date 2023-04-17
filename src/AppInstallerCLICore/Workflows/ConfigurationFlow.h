// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Composite flow that chooses what to do based on the installer type.
    // Required Args: None
    // Inputs: None
    // Outputs: ConfigurationProcessor
    void CreateConfigurationProcessor(Execution::Context& context);

    // Opens the configuration set.
    // Required Args: ConfigurationFile
    // Inputs: ConfigurationProcessor
    // Outputs: ConfigurationSet
    void OpenConfigurationSet(Execution::Context& context);

    // Outputs the configuration set.
    // Required Args: None
    // Inputs: ConfigurationSet
    // Outputs: None
    void ShowConfigurationSet(Execution::Context& context);

    // Outputs the configuration set.
    // Required Args: None
    // Inputs: ConfigurationProcessor, ConfigurationSet
    // Outputs: None
    void ShowConfigurationSetConflicts(Execution::Context& context);

    // Handles confirming the configuration set processing should proceed.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void ConfirmConfigurationProcessing(Execution::Context& context);

    // Applies the configuration set, showing progress as it proceeds.
    // Required Args: None
    // Inputs: ConfigurationProcessor, ConfigurationSet
    // Outputs: None
    void ApplyConfigurationSet(Execution::Context& context);
}
