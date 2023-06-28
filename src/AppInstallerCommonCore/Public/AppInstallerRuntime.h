// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerVersions.h>
#include <winget/LocIndependent.h>
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
        // Always one more than the last path; for being able to iterate paths in tests.
        Max
    };

    // The principal that an ACE applies to.
    enum class ACEPrincipal : uint32_t
    {
        CurrentUser,
        Admins,
        System,
    };

    // The permissions granted to a specific ACE.
    enum class ACEPermissions : uint32_t
    {
        // This is not "Deny All", but rather, "Not mentioned"
        None = 0x0,
        Read = 0x1,
        Write = 0x2,
        Execute = 0x4,
        ReadWrite = Read | Write,
        ReadExecute = Read | Execute,
        ReadWriteExecute = Read | Write | Execute,
        // All means that full control will be granted
        All = 0xFFFFFFFF
    };

    DEFINE_ENUM_FLAG_OPERATORS(ACEPermissions);

    // Information about a path that we use and how to set it up.
    struct PathDetails
    {
        std::filesystem::path Path;
        // Default to creating the directory with inherited ownership and permissions
        bool Create = true;
        std::optional<ACEPrincipal> Owner;
        std::map<ACEPrincipal, ACEPermissions> ACL;

        // Shorthand for setting Owner and giving them ACEPermissions::All
        void SetOwner(ACEPrincipal owner);

        // Determines if the ACL should be applied.
        bool ShouldApplyACL() const;

        // Applies the ACL unconditionally.
        void ApplyACL() const;
    };

    // Gets the PathDetails used for the given path.
    // This is exposed primarily to allow for testing, GetPathTo should be preferred.
    PathDetails GetPathDetailsFor(PathName path, bool forDisplay = false);

    // Gets the path to the requested location.
    std::filesystem::path GetPathTo(PathName path, bool forDisplay = false);

    // Gets a new temp file path.
    std::filesystem::path GetNewTempFilePath();

    // Determines whether developer mode is enabled.
    bool IsDevModeEnabled();

    // Gets the default user agent string for the Windows Package Manager.
    Utility::LocIndString GetDefaultUserAgent();

    // Gets the user agent string from passed in caller for the Windows Package Manager.
    Utility::LocIndString GetUserAgent(std::string_view caller);
}
