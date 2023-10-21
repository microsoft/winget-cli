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

    void InitiateRebootIfApplicable::operator()(Execution::Context& context) const
    {
        if (!Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Reboot))
        {
            return;
        }

        if (!context.Args.Contains(Execution::Args::Type::AllowReboot))
        {
            AICLI_LOG(CLI, Info, << "No reboot flag found; skipping reboot flow.");
            return;
        }

        if (WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::RebootRequired))
        {
            context.ClearFlags(Execution::ContextFlag::RebootRequired);

            if (Reboot::InitiateReboot())
            {
                context.Reporter.Warn() << Resource::String::InitiatingReboot << std::endl;
            }
            else
            {
                context.Reporter.Error() << Resource::String::FailedToInitiateReboot << std::endl;
            }
        }
    }
}
