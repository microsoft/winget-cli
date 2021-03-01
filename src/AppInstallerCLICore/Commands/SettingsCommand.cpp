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

    Resource::LocString SettingsCommand::ShortDescription() const
    {
        return { Resource::String::SettingsCommandShortDescription };
    }

    Resource::LocString SettingsCommand::LongDescription() const
    {
        return { Resource::String::SettingsCommandLongDescription };
    }

    std::string SettingsCommand::HelpLink() const
    {
        return "https://aka.ms/winget-settings";
    }

    void SettingsCommand::ExecuteInternal(Execution::Context& context) const
    {
        // Show warnings only when the setting command is executed.
        if (!User().GetWarnings().empty())
        {
            context.Reporter.Warn() << Resource::String::SettingLoadFailure << std::endl;
            for (const auto& warning : User().GetWarnings())
            {
                context.Reporter.Warn() << warning << std::endl;
            }
        }

        User().PrepareToShellExecuteFile();

        auto filePathUTF16 = UserSettings::SettingsFilePath().wstring();

        // Some versions of windows will fail if no file extension association exists, other will pop up the dialog
        // to make the user pick their default.
        // Kudos to the terminal team for this work around.
        HINSTANCE res = ShellExecuteW(nullptr, nullptr, filePathUTF16.c_str(), nullptr, nullptr, SW_SHOW);
        if (static_cast<int>(reinterpret_cast<uintptr_t>(res)) <= 32)
        {
            // User doesn't have file type association. Default to notepad
            AICLI_LOG(CLI, Info, << "Json file type association not found, using notepad.exe");
            ShellExecuteW(nullptr, nullptr, L"notepad", filePathUTF16.c_str(), nullptr, SW_SHOW);
        }
    }
}
