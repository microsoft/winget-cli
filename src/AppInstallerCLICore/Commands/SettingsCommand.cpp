// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SettingsCommand.h"
#include "Workflows/WorkflowBase.h"
#include "Workflows/SettingsFlow.h"

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
        Utility::LocIndString s_SettingsCommand_HelpLink = "https://aka.ms/winget-settings"_lis;
    }

    std::vector<std::unique_ptr<Command>> SettingsCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
            std::make_unique<SettingsExportCommand>(FullName()),
            });
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

    Utility::LocIndString SettingsCommand::HelpLink() const
    {
        return s_SettingsCommand_HelpLink;
    }

    void SettingsCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        if (execArgs.Contains(Execution::Args::Type::AdminSettingEnable) && execArgs.Contains(Execution::Args::Type::AdminSettingDisable))
        {
            throw CommandException(Resource::String::TooManyAdminSettingArgumentsError(s_ArgName_EnableAndDisable));
        }

        // Get admin setting string for all available options except Unknown
        using AdminSetting_t = std::underlying_type_t<AdminSetting>;
        std::vector<Utility::LocIndString> adminSettingList;
        for (AdminSetting_t i = 1 + static_cast<AdminSetting_t>(AdminSetting::Unknown); i < static_cast<AdminSetting_t>(AdminSetting::Max); ++i)
        {
            adminSettingList.emplace_back(AdminSettingToString(static_cast<AdminSetting>(i)));
        }

        Utility::LocIndString validOptions = Join(", "_liv, adminSettingList);

        if (execArgs.Contains(Execution::Args::Type::AdminSettingEnable) && AdminSetting::Unknown == StringToAdminSetting(execArgs.GetArg(Execution::Args::Type::AdminSettingEnable)))
        {
            throw CommandException(Resource::String::InvalidArgumentValueError(s_ArgumentName_Enable, validOptions));
        }

        if (execArgs.Contains(Execution::Args::Type::AdminSettingDisable) && AdminSetting::Unknown == StringToAdminSetting(execArgs.GetArg(Execution::Args::Type::AdminSettingDisable)))
        {
            throw CommandException(Resource::String::InvalidArgumentValueError(s_ArgumentName_Disable, validOptions));
        }
    }

    void SettingsCommand::ExecuteInternal(Execution::Context& context) const
    {
        if (context.Args.Contains(Execution::Args::Type::AdminSettingEnable))
        {
            context <<
                Workflow::EnsureRunningAsAdmin <<
                Workflow::EnableAdminSetting;

        }
        else if (context.Args.Contains(Execution::Args::Type::AdminSettingDisable))
        {
            context <<
                Workflow::EnsureRunningAsAdmin <<
                Workflow::DisableAdminSetting;
        }
        else
        {
            context << Workflow::OpenUserSetting;
        }
    }

    Resource::LocString SettingsExportCommand::ShortDescription() const
    {
        return { Resource::String::SettingsExportCommandShortDescription };
    }

    Resource::LocString SettingsExportCommand::LongDescription() const
    {
        return { Resource::String::SettingsExportCommandLongDescription };
    }

    Utility::LocIndString SettingsExportCommand::HelpLink() const
    {
        return s_SettingsCommand_HelpLink;
    }

    void SettingsExportCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::ExportSettings;
    }
}
