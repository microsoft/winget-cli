// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ResumeFlow.h"
#include "winget/Reboot.h"
#include <AppInstallerRuntime.h>

namespace AppInstaller::CLI::Workflow
{
    void Checkpoint::operator()(Execution::Context& context) const
    {
        if (!Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Resume))
        {
            return;
        }

        context.Checkpoint(m_checkpointName, m_contextData);
    }

    void RegisterStartupAfterReboot::operator()(Execution::Context& context) const
    {
        if (!Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Resume))
        {
            return;
        }

        if (WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::RegisterResume))
        {
            auto executablePath = AppInstaller::Runtime::GetPathTo(AppInstaller::Runtime::PathName::CLIExecutable);

            // RunOnce registry value must start with the full path of the executable.
            const auto& resumeId = context.GetResumeId();
            std::string commandLine = executablePath.u8string() + " resume -g " + resumeId;
            Reboot::WriteToRunOnceRegistry(resumeId, commandLine);
        }
    }
}
