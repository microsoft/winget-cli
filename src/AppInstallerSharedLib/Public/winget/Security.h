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

    // Creates a security descriptor granting all access to the current user with the mandatory label
    // set to the minimum integrity level given.
    // Does not work for Untrusted or ProtectedProcess integrity levels.
    wil::unique_hlocal_security_descriptor CreateSecurityDescriptorForCurrentUserWithMandatoryLabel(
        std::wstring_view mandatoryLabelRight,
        IntegrityLevel minimumIntegrityLevel);

    // Determines if the current COM caller is the same user as the current process
    // and is at least equal integrity level (higher will also be allowed).
    bool IsCOMCallerSameUserAndIntegrityLevel();

    // Gets the string representation of the given SID.
    std::string ToString(PSID sid);

    // Gets the wstring representation of the given SID.
    std::wstring ToWString(PSID sid);

    // Gets the SDDL wstring representation for the given integrity level.
    std::wstring_view ToWString(IntegrityLevel integrityLevel);
}
