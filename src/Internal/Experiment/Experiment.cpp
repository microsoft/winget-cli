// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Experiment.h"

namespace AppInstaller::Experiment
{
    bool IsEnabled(const ExperimentKey& key)
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (key == ExperimentKey::TestExperiment)
        {
            return true;
        }
#endif
        return false;
    }
}
