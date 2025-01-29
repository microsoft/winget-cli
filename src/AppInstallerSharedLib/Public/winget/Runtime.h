// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerVersions.h>
#include <winget/LocIndependent.h>

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace AppInstaller::Runtime
{
    // Determines whether the process is running in a packaged context or not.
    bool IsRunningInPackagedContext();

    // Determines the current version of the client and returns it.
    Utility::LocIndString GetClientVersion();

    // Gets the package family name of the current package (or empty string if not packaged).
    std::wstring GetPackageFamilyName();

    // Determines the current version of the package if running in a packaged context.
    Utility::LocIndString GetPackageVersion();

    // Gets a string representation of the OS version for debugging purposes.
    Utility::LocIndString GetOSVersion();

    // Gets the OS region.
    // This can be used as the current market.
    std::string GetOSRegion();

    // Determines whether the current OS version is >= the given one.
    // We treat the given Version struct as a standard 4 part Windows OS version.
    bool IsCurrentOSVersionGreaterThanOrEqual(const Utility::Version& version);

    // Determines whether the process is running with administrator privileges.
    bool IsRunningAsAdmin();

    // Determines whether the process is running with local system context.
    bool IsRunningAsSystem();

    // Determines whether the process is running with administrator or system privileges.
    bool IsRunningAsAdminOrSystem();

    // Determines whether the current token can be elevated.
    // This only returns true for tokens that are TokenElevationTypeLimited.
    // Thus, it will only be true if:
    //  1. UAC is enabled
    //  2. the user is in the Administrators group
    //  3. the token is not already elevated
    bool IsRunningWithLimitedToken();

    // Returns true if this is a release build; false if not.
    inline constexpr bool IsReleaseBuild()
    {
#ifdef WINGET_ENABLE_RELEASE_BUILD
        return true;
#else
        return false;
#endif
    }
}
