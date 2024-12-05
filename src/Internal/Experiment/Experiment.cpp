// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Experiment.h"

namespace AppInstaller::Experiment
{
    bool IsEnabled(const std::string& key)
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (key == "TestExperiment")
        {
            return true;
        }
#endif
        return false;
    }
}
