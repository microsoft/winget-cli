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
    };

    // Gets the path to the requested location.
    std::filesystem::path GetPathTo(PathName path);

    // Determines whether the current OS version is >= the given one.
    // We treat the given Version struct as a standard 4 part Windows OS version.
    bool IsCurrentOSVersionGreaterThanOrEqual(const Utility::Version& version);

    // Determines whether the process is running with administrator privileges.
    bool IsRunningAsAdmin();

    // Checks if the file system is NTFS
    bool IsNTFS(const std::filesystem::path& filePath);
}
