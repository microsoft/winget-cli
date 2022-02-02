// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "COMCommand.h"
#include "Workflows/DownloadFlow.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/UninstallFlow.h"
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
            Workflow::DownloadSinglePackage;
    }

    // IMPORTANT: To use this command, the caller should have already executed the COMDownloadCommand
    void COMInstallCommand::ExecuteInternal(Context& context) const
    {
        context <<
            Workflow::ReverifyInstallerHash <<
            Workflow::InstallPackageInstaller;
    }

    // IMPORTANT: To use this command, the caller should have already retrieved the InstalledPackageVersion and added it to the Context Data
    void COMUninstallCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::UninstallSinglePackage;
    }
}
