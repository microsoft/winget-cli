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
        Utility::LocIndView s_SettingsCommand_HelpLink = "https://aka.ms/winget-settings"_liv;
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
            Argument{ Execution::Args::Type::AdminSettingEnable, Resource::String::AdminSettingEnableDescription, ArgumentType::Standard, Argument::Visibility::Help },
            Argument{ Execution::Args::Type::AdminSettingDisable, Resource::String::AdminSettingDisableDescription, ArgumentType::Standard, Argument::Visibility::Help },
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

    Utility::LocIndView SettingsCommand::HelpLink() const
    {
        return s_SettingsCommand_HelpLink;
    }

    void SettingsCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
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
            throw CommandException(Resource::String::InvalidArgumentValueError(ArgumentCommon::ForType(Execution::Args::Type::AdminSettingEnable).Name, validOptions));
        }

        if (execArgs.Contains(Execution::Args::Type::AdminSettingDisable) && AdminSetting::Unknown == StringToAdminSetting(execArgs.GetArg(Execution::Args::Type::AdminSettingDisable)))
        {
            throw CommandException(Resource::String::InvalidArgumentValueError(ArgumentCommon::ForType(Execution::Args::Type::AdminSettingDisable).Name, validOptions));
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

    Utility::LocIndView SettingsExportCommand::HelpLink() const
    {
        return s_SettingsCommand_HelpLink;
    }

    void SettingsExportCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::ExportSettings;
    }
}
