// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace AppInstaller::Runtime
{
    // Determines whether the process is running in a packaged context or not.
    bool IsRunningInPackagedContext();

    // Determines the current version of the client and returns it.
    std::string GetClientVersion();

    // Gets the path to the temp location.
    std::filesystem::path GetPathToTemp();

    // Gets the path to the local state location.
    std::filesystem::path GetPathToLocalState();

    // Gets a stream containing the named setting's value, if present.
    // If the setting does not exist, returns an empty value.
    std::unique_ptr<std::istream> GetSettingStream(std::filesystem::path name);

    // Sets the named setting to the given value.
    void SetSetting(std::filesystem::path name, std::string_view value);

    // Deletes the given setting.
    void RemoveSetting(std::filesystem::path name);
}
