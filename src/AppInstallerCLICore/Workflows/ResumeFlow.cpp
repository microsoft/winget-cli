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

        context.Checkpoint(m_checkpointName, m_contextData);
    }
}
