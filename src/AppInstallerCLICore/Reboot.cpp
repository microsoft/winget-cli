// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Reboot.h"
#include "winget/Reboot.h"

namespace AppInstaller::CLI::Execution
{
    void InitiateRebootIfApplicable(Execution::Context& context)
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

        if (WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::RegisterForRestart))
        {
            // TODO: Register application for restart.
        }

        if (WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::RebootRequired))
        {
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
