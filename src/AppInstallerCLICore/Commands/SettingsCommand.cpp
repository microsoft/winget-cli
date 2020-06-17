// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SettingsCommand.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"
#include <winget/UserSettings.h>

namespace AppInstaller::CLI
{
    using namespace Utility::literals;
    using namespace AppInstaller::Settings;

    using namespace std::string_view_literals;

    std::vector<Argument> SettingsCommand::GetArguments() const
    {
        return {};
    }

    Resource::LocString SettingsCommand::ShortDescription() const
    {
        // TODO: Create a string and localize.
        return { Resource::String::HashCommandShortDescription };
    }

    Resource::LocString SettingsCommand::LongDescription() const
    {
        // TODO: Create a string and localize.
        return { Resource::String::HashCommandLongDescription };
    }

    std::string SettingsCommand::HelpLink() const
    {
        // TODO: Create and point to readme
        return "https://aka.ms/winget-settings";
    }

    void SettingsCommand::ExecuteInternal(Execution::Context& ) const
    {
        UserSettingsType userSettingType = UserSettings::Instance().GetType();

        if (userSettingType == UserSettingsType::Default)
        {
            // Create settings file if it doesn't exist.
            UserSettings::Instance().CreateFileIfNeeded();
        }
        else if (userSettingType == UserSettingsType::Standard)
        {
            // Settings file was loaded correctly, create backup.
            UserSettings::Instance().CreateBackup();
        }

        std::string filePathUTF8Str = UserSettings::Instance().SettingsFilePath().u8string();

        // Kudos to the terminal team for this work around.
        HINSTANCE res = ShellExecuteA(nullptr, nullptr, filePathUTF8Str.c_str(), nullptr, nullptr, SW_SHOW);
        if (static_cast<int>(reinterpret_cast<uintptr_t>(res)) <= 32)
        {
            // User doesn't have file type association. Default to notepad
            ShellExecuteA(nullptr, nullptr, "notepad", filePathUTF8Str.c_str(), nullptr, SW_SHOW);
        }
    }
}
