// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/GroupPolicy.h>

#include <string_view>


namespace AppInstaller::Repository
{
    // Checks whether the Group Policy allows this user source.
    // If it does it returns None, otherwise it returns which policy is blocking it.
    // Note that this applies to user sources that are being added as well as user sources
    // that already existed when the Group Policy came into effect.
    Settings::TogglePolicy::Policy GetPolicyBlockingUserSource(std::string_view name, std::string_view type, std::string_view arg, bool isTombstone);
}
