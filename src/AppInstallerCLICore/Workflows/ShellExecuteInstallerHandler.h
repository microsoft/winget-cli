// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerProgress.h>
#include "ExecutionContext.h"

#include <filesystem>
#include <optional>

// ShellExecuteInstallerHandler handles installers run through ShellExecute.
// Exe, Wix, Nullsoft, Msi and Inno should be handled by this installer handler.
namespace AppInstaller::CLI::Workflow
{
    // Install is done through invoking ShellExecute on downloaded installer.
    // Required Args: None
    // Inputs: Manifest?, InstallerPath, InstallerArgs
    // Outputs: OperationReturnCode
    void ShellExecuteInstallImpl(Execution::Context& context);

    // Uninstall is done through invoking ShellExecute on uninstall string.
    // Required Args: None
    // Inputs: UninstallString
    // Outputs: OperationReturnCode
    void ShellExecuteUninstallImpl(Execution::Context& context);

    // Removes the MSI
    // Required Args: None
    // Inputs: ProductCodes
    // Output: None
    void ShellExecuteMsiExecUninstall(Execution::Context& context);

    // Gets the installer args from the context.
    // Required Args: None
    // Inputs: Manifest?, Installer, InstallerPath
    // Outputs: InstallerArgs
    void GetInstallerArgs(Execution::Context& context);

    // Repair is done through invoking ShellExecute on downloaded installer.
    // Required Args: None
    // Inputs: Manifest?, InstallerPath, InstallerArgs
    // Outputs: OperationReturnCode
    void ShellExecuteRepairImpl(Execution::Context& context);

    // Repair the MSI
    // Required Args: None
    // Inputs: ProductCodes
    // Output: None
    void ShellExecuteMsiExecRepair(Execution::Context& context);

    // Enables the Windows Feature dependency by invoking ShellExecute on the DISM executable.
    // Required Args: None
    // Inputs: Windows Feature dependency
    // Outputs: None
    struct ShellExecuteEnableWindowsFeature : public WorkflowTask
    {
        ShellExecuteEnableWindowsFeature(std::string_view featureName) : WorkflowTask("ShellExecuteEnableWindowsFeature"), m_featureName(featureName) {}

        void operator()(Execution::Context& context) const override;

    private:
        std::string_view m_featureName;
    };

    // Extracts the installer archive using the tar executable.
    // Required Args: None
    // Inputs: InstallerPath
    // Outputs: None
    struct ShellExecuteExtractArchive : public WorkflowTask
    {
        ShellExecuteExtractArchive(const std::filesystem::path& archivePath, const std::filesystem::path& destPath) : WorkflowTask("ShellExecuteExtractArchive"), m_archivePath(archivePath), m_destPath(destPath) {}

        void operator()(Execution::Context& context) const override;

    private:
        std::filesystem::path m_archivePath;
        std::filesystem::path m_destPath;
    };
}
