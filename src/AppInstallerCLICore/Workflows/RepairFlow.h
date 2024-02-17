// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{

    // Internal implementation details
    namespace
    {
        void SetUninstallStringInContext(Execution::Context& context);

        void SetModifyPathInContext(Execution::Context& context);

        void SetProductCodesInContext(Execution::Context& context);

        void SetPackageFamilyNamesInContext(Execution::Context& context);

        /// <summary>
        /// The function performs a preliminary check on the installed package by reading its ARP registry flags for NoModify and NoRepair to confirm if the repair operation is applicable.
        /// </summary>
        /// <param name="context">Execution context object</param>
        void ApplicabilityCheckForInstalledPackage(Execution::Context& context);

        /// <summary>
        /// This function performs a preliminary check on the available matching package by reading its manifest entries for repair behavior to determine the type of repair operation and repair switch are applicable
        /// </summary>
        /// <param name="context">Execution context object</param>
        void ApplicabilityCheckForAvailablePackage(Execution::Context& context);

        void GenerateRepairString(Execution::Context& context);

        void RunRepairForRepairBehaviorBasedInstallers(Execution::Context& context);
    }

    void RepairApplicabilityCheck(Execution::Context& context);

    void ExecuteRepair(Execution::Context& context);

    void GetRepairInfo(Execution::Context& context);

    void RepairMsixPackage(Execution::Context& context);

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

    struct RepairSinglePackage : public WorkflowTask
    {
        RepairSinglePackage(OperationType operation) :
            WorkflowTask("RepairSinglePackage"), m_operationType(operation) {}

        void operator()(Execution::Context& context) const override;

    private:
        mutable OperationType m_operationType;
    };

    struct SelectApplicablePackageVersion : public WorkflowTask
    {
        SelectApplicablePackageVersion(bool isSinglePackage) :
            WorkflowTask("SelectApplicablePackageVersion"), m_isSinglePackage(isSinglePackage) {}

        void operator()(Execution::Context& context) const override;

    private:
        bool m_isSinglePackage;
    };

}
