// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    void RepairApplicabilityCheck(Execution::Context& context);

    void ExecuteRepair(Execution::Context& context);

    void GetRepairInfo(Execution::Context& context);

    void GenerateRepairString(Execution::Context& context);

    // Reports the result of the repair.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    struct ReportRepairResult : public WorkflowTask
    {
        ReportRepairResult(HRESULT hr, bool isHResult = false) :
            WorkflowTask("ReportRepairResult"),
            m_hr(hr),
            m_isHResult(isHResult) {}

        void operator()(Execution::Context& context) const override;

    private:
        HRESULT m_hr;
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
}
