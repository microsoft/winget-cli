// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "COMInstallCommand.h"
#include "Workflows/DependenciesFlow.h"
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
        // TODO: Remove duplication with InstallFlow
        context <<
            Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
            Workflow::SelectInstaller <<
            Workflow::EnsureApplicableInstaller <<
            Workflow::ReportIdentityAndInstallationDisclaimer <<
            Workflow::ShowPackageAgreements(/* ensureAcceptance */ true) <<
            Workflow::GetDependenciesFromInstaller <<
            Workflow::ReportDependencies(Resource::String::InstallAndUpgradeCommandsReportDependencies) <<
            Workflow::ReportExecutionStage(ExecutionStage::Download) <<
            Workflow::DownloadInstaller;
    }

    // IMPORTANT: To use this command, the caller should have already retrieved the package manifest (GetManifest()) and added it to the Context Data
    void COMInstallCommand::ExecuteInternal(Context& context) const
    {
        context <<
            Workflow::InstallPackageInstaller;
    }
}
