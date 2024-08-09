// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <filesystem>
#include <map>
#include <optional>
#include <shtypes.h>

namespace AppInstaller::Filesystem
{
    // Checks if the file system at path supports named streams/ADS
    bool SupportsNamedStreams(const std::filesystem::path& path);

    // Checks if the file system at path supports hard links
    bool SupportsHardLinks(const std::filesystem::path& path);

    // Checks if the file system at path support reparse points
    bool SupportsReparsePoints(const std::filesystem::path& path);

    // Checks if the canonical form of the path points to a location outside of the provided base path.
    bool PathEscapesBaseDirectory(const std::filesystem::path& target, const std::filesystem::path& base);

    // Renames the file to a new path.
    void RenameFile(const std::filesystem::path& from, const std::filesystem::path& to);

    // Creates a symlink that points to the target path.
    bool CreateSymlink(const std::filesystem::path& target, const std::filesystem::path& link);

    // Verifies that a symlink points to the target path.
    bool VerifySymlink(const std::filesystem::path& symlink, const std::filesystem::path& target);

    // Appends the .exe extension to the path if not present.
    void AppendExtension(std::filesystem::path& value, const std::string& extension);

    // Checks if the path is a symlink and exists.
    bool SymlinkExists(const std::filesystem::path& symlinkPath);
    bool CreateSymlink(const std::filesystem::path& path, const std::filesystem::path& target);

    // Get expanded file system path.
    std::filesystem::path GetExpandedPath(const std::string& path);

    // If `source` begins with all of `prefix`, replace that with `replacement`.
    // Returns true if replacement happened, false otherwise.
    bool ReplaceCommonPathPrefix(std::filesystem::path& source, const std::filesystem::path& prefix, std::string_view replacement);

    // Gets the path of a known folder.
    std::filesystem::path GetKnownFolderPath(const KNOWNFOLDERID& id);

    // Verifies that the paths are on the same volume.
    bool IsSameVolume(const std::filesystem::path& path1, const std::filesystem::path& path2);

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

    // Initializes from the given details and returns the path to it.
    // The path is moved out of the details.
    std::filesystem::path InitializeAndGetPathTo(PathDetails&& details);

    // Gets the path to the requested location.
    template <class PathEnum>
    std::filesystem::path GetPathTo(PathEnum path, bool forDisplay = false)
    {
        return InitializeAndGetPathTo(GetPathDetailsFor(path, forDisplay));
    }

    // A shared path.
    enum class PathName
    {
        // Local state root that is specifically unpackaged (even if used from a packaged process).
        UnpackagedLocalStateRoot,
        // Local settings root that is specifically unpackaged (even if used from a packaged process).
        UnpackagedSettingsRoot,
    };

    // Gets the PathDetails used for the given path.
    // This is exposed primarily to allow for testing, GetPathTo should be preferred.
    PathDetails GetPathDetailsFor(PathName path, bool forDisplay = false);
}
