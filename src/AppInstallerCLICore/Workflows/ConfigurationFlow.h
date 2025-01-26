// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Creates a configuration processor with a processor factory for full functionality.
    // Required Args: None
    // Inputs: None
    // Outputs: ConfigurationProcessor
    void CreateConfigurationProcessor(Execution::Context& context);

    // Creates a configuration processor without a processor factory for reduced functionality.
    // Required Args: None
    // Inputs: None
    // Outputs: ConfigurationProcessor
    void CreateConfigurationProcessorWithoutFactory(Execution::Context& context);

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

    // Search source for package(s) to be exported in configuration file.
    // Required Args: None
    // Inputs: None
    // Outputs: PackageCollection
    void SearchSourceForPackageExport(Execution::Context& context);

    // Adds configuration unit(s) with the winget package and/or exports resource given to configuration set.
    // Required Args: None
    // Inputs: ConfigurationProcessor, ConfigurationSet
    // Outputs: None
    void PopulateConfigurationSetForExport(Execution::Context& context);

    // Write the configuration file.
    // Required Args: OutputFile
    // Inputs: ConfigurationProcessor, ConfigurationSet
    // Outputs: None
    void WriteConfigFile(Execution::Context& context);

    // Gets the configuration set history.
    // Required Args: None
    // Inputs: ConfigurationProcessor
    // Outputs: ConfigurationSetHistory
    void GetConfigurationSetHistory(Execution::Context& context);

    // Outputs the configuration set history.
    // Required Args: None
    // Inputs: ConfigurationSetHistory
    // Outputs: None
    void ShowConfigurationSetHistory(Execution::Context& context);

    // Selects a specific configuration set history item.
    // Required Args: ConfigurationHistoryItem
    // Inputs: ConfigurationSetHistory
    // Outputs: ConfigurationSet
    void SelectSetFromHistory(Execution::Context& context);

    // Removes the configuration set from history.
    // Required Args: None
    // Inputs: ConfigurationSet
    // Outputs: None
    void RemoveConfigurationSetHistory(Execution::Context& context);

    // Write the configuration set history item to a file.
    // Required Args: OutputFile
    // Inputs: ConfigurationSet
    // Outputs: None
    void SerializeConfigurationSetHistory(Execution::Context& context);

    // Outputs a single configuration set (from history).
    // Required Args: None
    // Inputs: ConfigurationSet
    // Outputs: None
    void ShowSingleConfigurationSetHistory(Execution::Context& context);

    // Completes the configuration history item.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void CompleteConfigurationHistoryItem(Execution::Context& context);

    // Monitors configuration status.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void MonitorConfigurationStatus(Execution::Context& context);
}
