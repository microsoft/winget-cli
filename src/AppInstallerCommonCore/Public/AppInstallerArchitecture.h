// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <vector>
#include <winget/LocIndependent.h>

namespace AppInstaller::Utility
{
    static const int InapplicableArchitecture = -1;

    enum class Architecture
    {
        Unknown = -1,
        Neutral,
        X86,
        X64,
        Arm,
        Arm64
    };

    // Converts a string to corresponding enum
    Architecture ConvertToArchitectureEnum(const std::string& archStr);

    // Converts an Architecture to a string_view
    LocIndView ToString(Architecture architecture);

    // Gets the system's architecture as Architecture enum
    AppInstaller::Utility::Architecture GetSystemArchitecture();

    // Gets the set of architectures that are applicable to the current system
    const std::vector<Architecture>& GetApplicableArchitectures();

    // Gets if an architecture is applicable to the system
    // Returns the priority in the applicable architecture list if the architecture is applicable. 0 has lowest priority.
    // Returns -1 if the architecture is not applicable.
    int IsApplicableArchitecture(Architecture arch);

    // Gets if an architecture is applicable to the given list
    // Returns the priority in the applicable architecture list if the architecture is applicable. 0 has lowest priority.
    // Returns -1 if the architecture is not applicable.
    int IsApplicableArchitecture(Architecture arch, const std::vector<Architecture>& allowedArchitectures);
}
