// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace AppInstaller::Settings
{
    // Allows settings to be classified and treated differently base on any number of factors.
    // Names should still be unique, as there is no guarantee made about types mapping to unique roots.
    enum class Type
    {
        // A Standard setting stream has no special requirements.
        Standard,
        // A UserFile setting stream should be located in a file that is easily editable by the user.
        UserFile,
    };

    // Gets a stream containing the named setting's value, if present.
    // If the setting does not exist, returns an empty value.
    std::unique_ptr<std::istream> GetSettingStream(Type type, const std::filesystem::path& name);

    // Sets the named setting to the given value.
    void SetSetting(Type type, const std::filesystem::path& name, std::string_view value);

    // Deletes the given setting.
    void RemoveSetting(Type type, const std::filesystem::path& name);
}
