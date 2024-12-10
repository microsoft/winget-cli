// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerVersions.h>
#include <winget/LocIndependent.h>
#include <winget/Filesystem.h>
#include <winget/Runtime.h>

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace AppInstaller::Runtime
{
    // Sets the runtime path state name globally.
    void SetRuntimePathStateName(std::string name);

    // A path to be retrieved based on the runtime.
    enum class PathName
    {
        // The temporary file location.
        Temp,
        // The local state (file) storage location.
        LocalState,
        // The default location where log files are located.
        DefaultLogLocation,
        // The location that standard type settings are stored.
        // In a packaged context, this returns a prepend value for the container name.
        StandardSettings,
        // The location that user file type settings are stored.
        UserFileSettings,
        // The location where secure settings data is stored (for reading).
        SecureSettingsForRead,
        // The location where secure settings data is stored (for writing).
        SecureSettingsForWrite,
        // The value of %USERPROFILE%.
        UserProfile,
        // The location where portable packages are installed to with user scope.
        PortablePackageUserRoot,
        // The location where portable packages are installed to with machine scope.
        PortablePackageMachineRoot,
        // The location where portable packages are installed to with machine scope (x86).
        PortablePackageMachineRootX86,
        // The location where symlinks to portable packages are stored under user scope.
        PortableLinksUserLocation,
        // The location where symlinks to portable packages are stored under machine scope.
        PortableLinksMachineLocation,
        // The root location for the package containing the winget application.
        SelfPackageRoot,
        // The location where user downloads are stored.
        UserProfileDownloads,
        // The location where configuration modules are stored.
        ConfigurationModules,
        // The location where checkpoints are stored.
        CheckpointsLocation,
        // The location of the CLI executable file.
        CLIExecutable,
        // The location of the image assets, if it exists.
        ImageAssets,
        // The location where fonts are installed with user scope.
        FontsUserInstallLocation,
        // The location where fonts are installed with machine scope.
        FontsMachineInstallLocation,
        // Always one more than the last path; for being able to iterate paths in tests.
        Max
    };

    // Gets the PathDetails used for the given path.
    // This is exposed primarily to allow for testing, GetPathTo should be preferred.
    Filesystem::PathDetails GetPathDetailsFor(PathName path, bool forDisplay = false);

    // Gets the path to the requested location.
    inline std::filesystem::path GetPathTo(PathName path, bool forDisplay = false)
    {
        return Filesystem::GetPathTo(path, forDisplay);
    }

    // Replaces the substring in the path with the user profile environment variable.
    void ReplaceProfilePathsWithEnvironmentVariable(std::filesystem::path& path);

    // Gets a new temp file path.
    std::filesystem::path GetNewTempFilePath();

    // Determines whether developer mode is enabled.
    bool IsDevModeEnabled();

    // Gets the default user agent string for the Windows Package Manager.
    Utility::LocIndString GetDefaultUserAgent();

    // Gets the user agent string from passed in caller for the Windows Package Manager.
    Utility::LocIndString GetUserAgent(std::string_view caller);
}
