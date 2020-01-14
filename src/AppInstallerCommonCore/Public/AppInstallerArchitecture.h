// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <set>

namespace AppInstaller::Utility
{
    enum class Architecture
    {
        unknown = -1,
        neutral,
        x86,
        x64,
        arm,
        arm64
    };

    // Converts a string to corresponding enum
    Architecture ConvertToArchitectureEnum(const std::string& archStr);

    // Gets a set of architectures that are applicable to the current system
    std::set<Architecture> GetApplicableArchitectures();

    // Gets if an architecture is applicable to the system
    bool IsApplicableArchitecture(Architecture arch);
}