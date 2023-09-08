// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ResumeFlow.h"

namespace AppInstaller::CLI::Workflow
{
    void Checkpoint::operator()(Execution::Context& context) const
    {
        if (!Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Resume))
        {
            return;
        }

        if (WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::Resume))
        {
            // TODO: If the resume flag is set, resume workflow behavior.
        }
        else
        {
        }
    }
}
