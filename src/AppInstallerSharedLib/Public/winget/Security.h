// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <wil/resource.h>
#include <string>
#include <string_view>

namespace AppInstaller::Security
{
    // A Windows integrity level.
    enum class IntegrityLevel
    {
        Untrusted,
        Low,
        Medium,
        MediumPlus,
        High,
        System,
        ProtectedProcess,
    };

    // Gets the integrity level for the current effective token.
    // Does not know how to determine MediumPlus, if that ever matters...
    IntegrityLevel GetEffectiveIntegrityLevel();

    // Determines if the current COM caller is the same user as the current process
    // and is at least equal integrity level (higher will also be allowed).
    bool IsCOMCallerSameUserAndIntegrityLevel();

    // Determines if the current COM caller is at least the minimum integrity level provided.
    bool IsCOMCallerIntegrityLevelAtLeast(IntegrityLevel minimumLevel);

    // Determines if the current integrity level is at least the minimum integrity level provided.
    bool IsCurrentIntegrityLevelAtLeast(IntegrityLevel minimumLevel);

    // Gets the string representation of the given SID.
    std::string ToString(PSID sid);
}
