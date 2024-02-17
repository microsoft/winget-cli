// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{

    // Internal implementation details
    namespace
    {
        // Sets the uninstall string in the context.
        // RequiredArgs:
        // Inputs:InstalledPackageVersion
        // Outputs:SilentUninstallString, UninstallString
        void SetUninstallStringInContext(Execution::Context& context);

        // Sets the modify path in the context.
        // RequiredArgs:None
        // Inputs:InstalledPackageVersion
        // Outputs:ModifyPath
        void SetModifyPathInContext(Execution::Context& context);

        // Sets the product codes in the context.
        // RequiredArgs:None
        // Inputs:InstalledPackageVersion
        // Outputs:ProductCodes
        void SetProductCodesInContext(Execution::Context& context);

        // Sets the package family names in the context.
        // RequiredArgs:None
        // Inputs:InstalledPackageVersion
        // Outputs:PackageFamilyNames
        void SetPackageFamilyNamesInContext(Execution::Context& context);

        // The function performs a preliminary check on the installed package by reading its ARP registry flags for NoModify and NoRepair to confirm if the repair operation is applicable.
        // RequiredArgs:None
        // Inputs:InstalledPackageVersion, NoModify ?, NoRepair ?
        // Outputs:None
        void ApplicabilityCheckForInstalledPackage(Execution::Context& context);

        // This function performs a preliminary check on the available matching package by reading its manifest entries for repair behavior to determine the type of repair operation and repair switch are applicable
        // RequiredArgs:None
        // Inputs:InstallerType, RepairBehavior
        // Outputs:None
        void ApplicabilityCheckForAvailablePackage(Execution::Context& context);

        // Generate the repair string based on the repair behavior and installer type.
        // RequiredArgs:None
        // Inputs:BaseInstallerType, RepairBehavior, ModifyPath?, UninstallString?, InstallerArgs
        // Outputs:RepairString
        void GenerateRepairString(Execution::Context& context);

        // Execute the repair operation for RepairBehavior based installers.
        // RequiredArgs:None
        // Inputs: RepairBehavior, RepairString
        // Outputs:None
        void RunRepairForRepairBehaviorBasedInstallers(Execution::Context& context);
    }

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

    // Perform the repair operation for the MSIX package.
    // RequiredArgs:None
    // Inputs:PackageFamilyNames , InstallScope?
    // Outputs:None
    void RepairMsixPackage(Execution::Context& context);

    // Select the applicable package version by matching the installed package version with the available package version.
    // RequiredArgs:None
    // Inputs: Package,InstalledPackageVersion, AvailablePackageVersions
    // Outputs:Manifest, PackageVersion, Installer
    void SelectApplicablePackageVersion(Execution::Context& context);

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
