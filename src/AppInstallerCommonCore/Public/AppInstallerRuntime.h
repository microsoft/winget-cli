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

    // Determines the current version of the package if running in a packaged context.
    Utility::LocIndString GetPackageVersion();

    // Gets a string representation of the OS version for debugging purposes.
    Utility::LocIndString GetOSVersion();

    // Gets the OS region.
    // This can be used as the current market.
    std::string GetOSRegion();

    // A path to be retrieved based on the runtime.
    enum class PathName
    {
        // The temporary file location.
        Temp,
        // The local state (file) storage location.
        LocalState,
        // The default location where log files are located.
        DefaultLogLocation,
        // The default location, anonymized using environment variables.
        DefaultLogLocationForDisplay,
        // The location that standard type settings are stored.
        // In a packaged context, this returns a prepend value for the container name.
        StandardSettings,
        // The location that user file type settings are stored.
        UserFileSettings,
        // The location where secure settings data is stored.
        SecureSettings,
        // The value of %USERPROFILE%.
        UserProfile,
        // The location where portable packages are installed to with user scope.
        PortablePackageUserRoot,
        // The location where portable packages are installed to with machine scope (x64).
        PortablePackageMachineRootX64,
        // The location where portable packages are installed to with machine scope (x86).
        PortablePackageMachineRootX86,
        // The location where symlinks to portable packages are stored under user scope.
        PortableLinksUserLocation,
        // The location where symlinks to portable packages are stored under machine scope.
        PortableLinksMachineLocation,
    };

    void SetRuntimePathStateName(std::string name);

    // Gets the path to the requested location.
    std::filesystem::path GetPathTo(PathName path);

    // Gets a new temp file path.
    std::filesystem::path GetNewTempFilePath();

    // Determines whether the current OS version is >= the given one.
    // We treat the given Version struct as a standard 4 part Windows OS version.
    bool IsCurrentOSVersionGreaterThanOrEqual(const Utility::Version& version);

    // Determines whether the process is running with administrator privileges.
    bool IsRunningAsAdmin();

    // Returns true if this is a release build; false if not.
    inline constexpr bool IsReleaseBuild();

    // Gets the default user agent string for the Windows Package Manager.
    Utility::LocIndString GetDefaultUserAgent();
}
