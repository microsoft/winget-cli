// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ResumeFlow.h"
#include "winget/Reboot.h"

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

    void RegisterForReboot::operator()(Execution::Context& context) const
    {
        if (!Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Resume) && 
            !Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Reboot))
        {
            return;
        }

        if (WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::RegisterResume))
        {
            std::string commandLineArg = "resume -g " + context.GetResumeId();

            if (!Reboot::RegisterApplicationForReboot(commandLineArg))
            {
                context.Reporter.Error() << Resource::String::FailedToRegisterReboot << std::endl;

                if (WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::RebootRequired))
                {
                    context.ClearFlags(Execution::ContextFlag::RebootRequired);
                }
            }
        }
    }
}
