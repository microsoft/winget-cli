// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Execute the repair operation for RepairBehavior based installers.
    // RequiredArgs:None
    // Inputs: RepairBehavior, RepairString
    // Outputs:None
    void RunRepairForRepairBehaviorBasedInstaller(Execution::Context& context);

    // Execute the repair operation for MSI based installers.
    // RequiredArgs:None
    // Inputs: ProductCodes
    // Outputs:None
    void RepairMsiBasedInstaller(Execution::Context& context);

    // Applicability check for repair operation.
    // RequiredArgs:None
    // Inputs:InstalledPackageVersion, NoModify ?, NoRepair ?
    // Outputs:None
    void RepairApplicabilityCheck(Execution::Context& context);

    // Execute the repair operation.
    // RequiredArgs:None
    // Inputs: InstallerType, RepairBehavior ?, RepairString? , ProductCodes?, PackageFamilyNames?
    // Outputs:None
    void ExecuteRepair(Execution::Context& context);

    // Obtains the necessary information for repair operation.
    // RequiredArgs:None
    // Inputs:InstallerType
    // Outputs:RepairString?, ProductCodes?, PackageFamilyNames?
    void GetRepairInfo(Execution::Context& context);

    // Perform the repair operation for the MSIX NonStore package.
    // RequiredArgs:None
    // Inputs:PackageFamilyNames , InstallScope?
    // Outputs:None
    void RepairMsixPackage(Execution::Context& context);

    // Select the applicable package version by matching the installed package version with the available package version.
    // RequiredArgs:None
    // Inputs: Package,InstalledPackageVersion, AvailablePackageVersions
    // Outputs:Manifest, PackageVersion, Installer
    void SelectApplicablePackageVersion(Execution::Context& context);

    /// <summary>
    /// Select the applicable installer for the installed package if necessary.
    // RequiredArgs:None
    // Inputs: Package,InstalledPackageVersion, AvailablePackageVersions
    // Outputs:Manifest, PackageVersion, Installer
    void SelectApplicableInstallerIfNecessary(Execution::Context& context);

    // Perform the repair operation for the single package.
    // RequiredArgs:None
    // Inputs: SearchResult, InstalledPackage, ApplicableInstaller
    // Outputs:None
    void RepairSinglePackage(Execution::Context& context);

    // Reports the result of the repair.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    struct ReportRepairResult : public WorkflowTask
    {
        ReportRepairResult(std::string_view repairType, HRESULT hr, bool isHResult = false) :
            WorkflowTask("ReportRepairResult"),
            m_repairType(repairType),
            m_hr(hr),
            m_isHResult(isHResult) {}

        void operator()(Execution::Context& context) const override;

    private:
        // Repair type used for reporting failure.
        std::string_view m_repairType;
        // Result to return if the repair fails.
        HRESULT m_hr;
        // Whether the result is an HRESULT.
        bool m_isHResult;
    };
}
