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

    // Creates or opens the configuration set.
    // Required Args: OutputFile
    // Inputs: ConfigurationProcessor
    // Outputs: ConfigurationSet
    void CreateOrOpenConfigurationSet(Execution::Context& context);

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
    struct ConfirmConfigurationProcessing : public WorkflowTask
    {
        ConfirmConfigurationProcessing(bool isApply) : WorkflowTask("ConfirmConfigurationProcessing"), m_isApply(isApply) {}

        void operator()(Execution::Context& context) const override;

    private:
        bool m_isApply;
    };

    // Applies the configuration set, showing progress as it proceeds.
    // Required Args: None
    // Inputs: ConfigurationProcessor, ConfigurationSet
    // Outputs: None
    void ApplyConfigurationSet(Execution::Context& context);

    // Tests the configuration set state, showing progress as it proceeds.
    // Required Args: None
    // Inputs: ConfigurationProcessor, ConfigurationSet
    // Outputs: None
    void TestConfigurationSet(Execution::Context& context);

    // Verifies the full package is installed.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void VerifyIsFullPackage(Execution::Context& context);

    // Validates the configuration set semantically.
    // Required Args: None
    // Inputs: ConfigurationProcessor, ConfigurationSet
    // Outputs: None
    void ValidateConfigurationSetSemantics(Execution::Context& context);

    // Validates that the unit processors referenced by the set are valid/available/etc.
    // Required Args: None
    // Inputs: ConfigurationProcessor, ConfigurationSet
    // Outputs: None
    void ValidateConfigurationSetUnitProcessors(Execution::Context& context);

    // Validates that specific unit contents referenced by the set are valid/available/etc.
    // Required Args: None
    // Inputs: ConfigurationProcessor, ConfigurationSet
    // Outputs: None
    void ValidateConfigurationSetUnitContents(Execution::Context& context);

    // Outputs the final message stating that no issues were found.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void ValidateAllGoodMessage(Execution::Context& context);

    // Adds a configuration unit with the winget package and/or exports resource given.
    // Required Args: None
    // Inputs: ConfigurationProcessor, ConfigurationSet
    // Outputs: None
    void AddWinGetPackageAndResource(Execution::Context& context);

    // Write the configuration file.
    // Required Args: OutputFile
    // Inputs: ConfigurationProcessor, ConfigurationSet
    // Outputs: None
    void WriteConfigFile(Execution::Context& context);
}
