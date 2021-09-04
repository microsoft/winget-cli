// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "COMInstallCommand.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/WorkflowBase.h"

using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Utility::literals;

namespace AppInstaller::CLI
{
    // IMPORTANT: To use this command, the caller should have already retrieved the package manifest (GetManifest()) and added it to the Context Data
    void COMDownloadCommand::ExecuteInternal(Context& context) const
    {
        context <<
            Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
            Workflow::SelectInstaller <<
            Workflow::EnsureApplicableInstaller <<
            Workflow::DownloadPackageVersion;
    }

    // IMPORTANT: To use this command, the caller should have already retrieved the package manifest (GetManifest()) and added it to the Context Data
    void COMInstallCommand::ExecuteInternal(Context& context) const
    {
        context <<
            Workflow::InstallPackageInstaller;
    }

    bool COMDownloadCommand::IsCommandAllowedToRunNow(std::map<std::string, UINT32>&, UINT32 runningCommandsOfCurrentType) const
    {
        return (runningCommandsOfCurrentType < 5);
    }
    
    bool COMInstallCommand::IsCommandAllowedToRunNow(std::map<std::string, UINT32>&, UINT32 runningCommandsOfCurrentType) const
    {
        return (runningCommandsOfCurrentType == 0);
    }
}
