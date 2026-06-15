// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SourceList.h"
#include <winget/GroupPolicy.h>

#include <string_view>


namespace AppInstaller::Repository
{
    // Checks whether the Group Policy allows this user source.
    // If it does it returns None, otherwise it returns which policy is blocking it.
    // Note that this applies to user sources that are being added as well as user sources
    // that already existed when the Group Policy came into effect.
    Settings::TogglePolicy::Policy GetPolicyBlockingUserSource(std::string_view name, std::string_view type, std::string_view arg, bool isTombstone);

    // Helper that converts the result of GetPolicyBlockingUserSource into a bool.
    bool IsUserSourceAllowedByPolicy(std::string_view name, std::string_view type, std::string_view arg, bool isTombstone);

    // Determines if a well known source is enabled; if onlyExplicit is true, it must be explicitly enabled by group policy.
    bool IsWellKnownSourceEnabled(WellKnownSource source, bool onlyExplicit = false);

    // Checks that the specified source is removable per policy; throwing if it is not.
    void EnsureSourceIsRemovable(const SourceDetailsInternal& source);
}
