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

    namespace
    {
        constexpr Utility::LocIndView s_ArgumentName_Enable = "enable"_liv;
        constexpr Utility::LocIndView s_ArgumentName_Disable = "disable"_liv;
        constexpr Utility::LocIndView s_ArgName_EnableAndDisable = "enable|disable"_liv;
    }

    std::vector<Argument> SettingsCommand::GetArguments() const
    {
        return {
            Argument{ s_ArgumentName_Enable, Argument::NoAlias, Execution::Args::Type::AdminSettingEnable, Resource::String::AdminSettingEnableDescription, ArgumentType::Standard, Argument::Visibility::Help },
            Argument{ s_ArgumentName_Disable, Argument::NoAlias, Execution::Args::Type::AdminSettingDisable, Resource::String::AdminSettingDisableDescription, ArgumentType::Standard, Argument::Visibility::Help },
        };
    }

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

    void SettingsCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        if (execArgs.Contains(Execution::Args::Type::AdminSettingEnable) && execArgs.Contains(Execution::Args::Type::AdminSettingDisable))
        {
            throw CommandException(Resource::String::TooManyAdminSettingArgumentsError, s_ArgName_EnableAndDisable);
        }

        if (execArgs.Contains(Execution::Args::Type::AdminSettingEnable) && AdminSetting::Unknown == StringToAdminSetting(execArgs.GetArg(Execution::Args::Type::AdminSettingEnable)))
        {
            throw CommandException(Resource::String::InvalidArgumentValueError, s_ArgumentName_Enable, { "LocalManifestFiles"_lis });
        }

        if (execArgs.Contains(Execution::Args::Type::AdminSettingDisable) && AdminSetting::Unknown == StringToAdminSetting(execArgs.GetArg(Execution::Args::Type::AdminSettingDisable)))
        {
            throw CommandException(Resource::String::InvalidArgumentValueError, s_ArgumentName_Disable, { "LocalManifestFiles"_lis });
        }
    }

    void SettingsCommand::ExecuteInternal(Execution::Context& context) const
    {
        if (context.Args.Contains(Execution::Args::Type::AdminSettingEnable))
        {
            context << Workflow::EnsureRunningAsAdmin;

            if (!context.IsTerminated())
            {
                Settings::EnableAdminSetting(StringToAdminSetting(context.Args.GetArg(Execution::Args::Type::AdminSettingEnable)));
                context.Reporter.Info() << Resource::String::AdminSettingEnabled;
            }
        }
        else if (context.Args.Contains(Execution::Args::Type::AdminSettingDisable))
        {
            context << Workflow::EnsureRunningAsAdmin;

            if (!context.IsTerminated())
            {
                Settings::DisableAdminSetting(StringToAdminSetting(context.Args.GetArg(Execution::Args::Type::AdminSettingDisable)));
                context.Reporter.Info() << Resource::String::AdminSettingDisabled;
            }
        }
        else
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
                            warn << ' ' << Resource::String::SettingsWarningField << ' ' << warning.Path;
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
                            warn << ' ' << Resource::String::SettingsWarningValue << ' ' << warning.Data;
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
}
