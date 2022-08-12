// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SettingsFlow.h"
#include <winget/UserSettings.h>
#include <winget/AdminSettings.h>
#include "Resources.h"

namespace AppInstaller::CLI::Workflow
{
    using namespace AppInstaller::Settings;

    void EnableAdminSetting(Execution::Context& context)
    {
        Settings::EnableAdminSetting(Settings::StringToAdminSetting(context.Args.GetArg(Execution::Args::Type::AdminSettingEnable)));
        context.Reporter.Info() << Resource::String::AdminSettingEnabled;
    }

    void DisableAdminSetting(Execution::Context& context)
    {
        Settings::DisableAdminSetting(Settings::StringToAdminSetting(context.Args.GetArg(Execution::Args::Type::AdminSettingDisable)));
        context.Reporter.Info() << Resource::String::AdminSettingDisabled;
    }

    void OpenUserSetting(Execution::Context& context)
    {
        // Show warnings only when the setting command is executed.
        if (!User().GetWarnings().empty())
        {
            context.Reporter.Warn() << Resource::String::SettingLoadFailure << std::endl;
            for (const auto& warning : User().GetWarnings())
            {
                auto warn = context.Reporter.Warn();
                warn << warning.Message;
                if (!warning.Path.empty())
                {
                    if (warning.IsFieldWarning)
                    {
                        warn << ' ' << Resource::String::SettingsWarningField(warning.Path);
                    }
                    else
                    {
                        warn << ' ' << warning.Path;
                    }
                }

                if (!warning.Data.empty())
                {
                    if (warning.IsFieldWarning)
                    {
                        warn << ' ' << Resource::String::SettingsWarningValue(warning.Data);
                    }
                    else
                    {
                        warn <<
                            std::endl <<
                            warning.Data;
                    }
                }

                warn << std::endl;
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
