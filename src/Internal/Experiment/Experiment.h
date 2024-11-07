// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <string>

namespace AppInstaller::Experiment
{
    bool IsEnabled(const std::string& experimentKey, bool overrideResult = false);
}
