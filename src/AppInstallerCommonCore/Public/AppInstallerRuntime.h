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

    // Gets the path to the temp location.
    std::filesystem::path GetPathToTemp();

    // Gets the path to the local state location.
    std::filesystem::path GetPathToLocalState();

    // Gets the path to the default log location.
    std::filesystem::path GetPathToDefaultLogLocation();

    // Gets a stream containing the named setting's value, if present.
    // If the setting does not exist, returns an empty value.
    std::unique_ptr<std::istream> GetSettingStream(std::filesystem::path name);

    // Sets the named setting to the given value.
    void SetSetting(std::filesystem::path name, std::string_view value);

    // Deletes the given setting.
    void RemoveSetting(std::filesystem::path name);

    // Determines whether the current OS version is >= the given one.
    // We treat the given Version struct as a standard 4 part Windows OS version.
    bool IsCurrentOSVersionGreaterThanOrEqual(const Utility::Version& version);
}
