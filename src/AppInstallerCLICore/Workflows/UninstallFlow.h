// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Uninstalls a single package. This also does the reporting, user interaction, and recording
    // for single-package uninstallation.
    // RequiredArgs: None
    // Inputs: InstalledPackageVersion
    // Outputs: None
    void UninstallSinglePackage(Execution::Context& context);

    // Gets the command string or package family names used to uninstall the package.
    // Required Args: None
    // Inputs: InstalledPackageVersion
    // Output: UninstallString?, PackageFamilyNames?
    void GetUninstallInfo(Execution::Context& context);

    // Uninstalls the package according to its type.
    // Required Args: None
    // Inputs: InstalledPackageVersion, UninstallString?, PackageFamilyNames?
    // Output: None
    void ExecuteUninstaller(Execution::Context& context);

    // Removes the MSIX.
    // Required Args: None
    // Inputs: PackageFamilyNames
    // Outputs: None
    void MsixUninstall(Execution::Context& context);

    // Records the uninstall to the tracking catalog.
    // Required Args: None
    // Inputs: Package
    // Outputs: None
    void RecordUninstall(Execution::Context& context);

    // Reports the return code returned by the Uninstaller.
    // Required Args: None
    // Inputs: InstalledPackageVersion
    // Outputs: None
    struct ReportUninstallerResult : public WorkflowTask
    {
        ReportUninstallerResult(std::string_view uninstallerType, HRESULT hr, bool isHResult = false) :
            WorkflowTask("ReportUninstallerResult"),
            m_uninstallerType(uninstallerType),
            m_hr(hr),
            m_isHResult(isHResult) {}

        void operator()(Execution::Context& context) const override;

    private:
        // Uninstaller type used when reporting failures.
        std::string_view m_uninstallerType;
        // Result to return if the Uninstaller failed.
        HRESULT m_hr;
        // Whether the Uninstaller result is an HRESULT. This guides how we show it.
        bool m_isHResult;
    };
}
