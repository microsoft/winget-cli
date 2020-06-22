// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace AppInstaller::Settings
{
    using namespace std::string_view_literals;

    // Allows settings to be classified and treated differently base on any number of factors.
    // Names should still be unique, as there is no guarantee made about types mapping to unique roots.
    enum class Type
    {
        // A Standard setting stream has no special requirements.
        Standard,
        // A UserFile setting stream should be located in a file that is easily editable by the user.
        UserFile,
        // A settings stream that should not be modified except by admin privileges.
        Secure,
    };

    // Converts the Type enum to a string.
    std::string_view ToString(Type type);

    // A stream definition, combining both type and path.
    // The well known values in Streams should be used by product code, while tests may directly create them.
    struct StreamDefinition
    {
        constexpr StreamDefinition(Type type, std::string_view path) : Type(type), Path(path) {}

        Type Type;
        std::string_view Path;
    };

    // The set of well known settings streams.
    // Changing these values can result in data loss.
    struct Streams
    {
        // The set of sources as defined by the user.
        constexpr static StreamDefinition UserSources{ Type::Secure, "user_sources"sv };
        // The metadata about all sources.
        constexpr static StreamDefinition SourcesMetadata{ Type::Standard, "sources_metadata"sv };
        // The primary user settings file.
        constexpr static StreamDefinition PrimaryUserSettings{ Type::UserFile, "settings.json"sv };
        // The backup user settings file.
        constexpr static StreamDefinition BackupUserSettings{ Type::UserFile, "settings.json.backup"sv };
    };

    // Gets a stream containing the named setting's value, if present.
    // If the setting does not exist, returns an empty value.
    std::unique_ptr<std::istream> GetSettingStream(const StreamDefinition& def);

    // Sets the named setting to the given value.
    void SetSetting(const StreamDefinition& def, std::string_view value);

    // Deletes the given setting.
    void RemoveSetting(const StreamDefinition& def);

    // Gets the path to the given stream definition.
    std::filesystem::path GetPathTo(const StreamDefinition& def);
}
