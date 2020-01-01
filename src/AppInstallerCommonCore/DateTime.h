// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <chrono>
#include <ostream>

namespace AppInstaller::Utility
{
    // Writes the given time to the given stream.
    // Assumes that system_clock uses Linux epoch (as required by C++20 standard).
    // Time is also assumed to be after the epoch.
    void OutputTimepoint(std::ostream& stream, const std::chrono::system_clock::time_point& time);
}
