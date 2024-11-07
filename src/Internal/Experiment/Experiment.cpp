// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Experiment.h"

namespace AppInstaller::Experiment
{
    bool IsEnabled(const std::string&, bool overrideResult)
    {
        return overrideResult;
    }
}
