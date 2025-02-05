// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <string>

namespace AppInstaller::Experiment
{
    enum class ExperimentKey : unsigned
    {
        None = 0,
        CDN,
        Max,

#ifndef AICLI_DISABLE_TEST_HOOKS
        TestExperiment = 0xFFFFFFFF,
#endif
    };

    bool IsEnabled(const ExperimentKey& experimentKey);
}
