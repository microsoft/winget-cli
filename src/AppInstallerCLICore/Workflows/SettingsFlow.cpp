// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Resources.h"
#include "SettingsFlow.h"
#include <AppInstallerStrings.h>
#include <winget/UserSettings.h>
#include <winget/AdminSettings.h>

namespace AppInstaller::CLI::Workflow
{
    using namespace AppInstaller::Settings;
    using namespace AppInstaller::Utility;

    namespace
    {
        struct ExportSettingsJson
        {
            ExportSettingsJson()
            {
                root["$schema"] = "https://aka.ms/winget-settings-export.schema.json";
                root["adminSettings"] = Json::ValueType::objectValue;
                root["userSettingsFile"] = UserSettings::SettingsFilePath().u8string();
            }

            void AddAdminSetting(AdminSetting setting)
            {
                auto str = std::string{ Settings::AdminSettingToString(setting) };
                root["adminSettings"][str] = Settings::IsAdminSettingEnabled(setting);
            }

            std::string ToJsonString() const
            {
                Json::StreamWriterBuilder writerBuilder;
                writerBuilder.settings_["indentation"] = "";
                return Json::writeString(writerBuilder, root);
            }

        private:
            Json::Value root{ Json::ValueType::objectValue };
        };
    }

    void EnableAdminSetting(Execution::Context& context)
    {
        auto adminSettingString = context.Args.GetArg(Execution::Args::Type::AdminSettingEnable);
        AdminSetting adminSetting = Settings::StringToAdminSetting(adminSettingString);
        if (Settings::EnableAdminSetting(adminSetting))
        {
            context.Reporter.Info() << Resource::String::AdminSettingEnabled(AdminSettingToString(adminSetting)) << std::endl;
        }
        else
        {
            context.Reporter.Error() << Resource::String::EnableAdminSettingFailed(AdminSettingToString(adminSetting)) << std::endl;
        }
    }

    void DisableAdminSetting(Execution::Context& context)
    {
        auto adminSettingString = context.Args.GetArg(Execution::Args::Type::AdminSettingDisable);
        AdminSetting adminSetting = Settings::StringToAdminSetting(adminSettingString);
        if (Settings::DisableAdminSetting(adminSetting))
        {
            context.Reporter.Info() << Resource::String::AdminSettingDisabled(AdminSettingToString(adminSetting)) << std::endl;
        }
        else
        {
            context.Reporter.Error() << Resource::String::DisableAdminSettingFailed(AdminSettingToString(adminSetting)) << std::endl;
        }
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
        // Kudos to the terminal team for this workaround.
        HINSTANCE res = ShellExecuteW(nullptr, nullptr, filePathUTF16.c_str(), nullptr, nullptr, SW_SHOW);
        if (static_cast<int>(reinterpret_cast<uintptr_t>(res)) <= 32)
        {
            // User doesn't have file type association. Default to notepad
            AICLI_LOG(CLI, Info, << "Json file type association not found, using notepad.exe");
            ShellExecuteW(nullptr, nullptr, L"notepad", filePathUTF16.c_str(), nullptr, SW_SHOW);
        }
    }

    void ExportSettings(Execution::Context& context)
    {
        ExportSettingsJson exportSettingsJson;

        for (const auto& setting : GetAllAdminSettings())
        {
            exportSettingsJson.AddAdminSetting(setting);
        }

        context.Reporter.Info() << exportSettingsJson.ToJsonString() << std::endl;
    }
}
