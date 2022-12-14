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
        constexpr Utility::LocIndView s_ArgValue_EnableAndDisable_LocalManifestFiles = "LocalManifestFiles"_liv;
        static constexpr std::string_view s_SettingsCommand_HelpLink = "https://aka.ms/winget-settings"sv;
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

    std::string SettingsCommand::HelpLink() const
    {
        return std::string{ s_SettingsCommand_HelpLink };
    }

    void SettingsCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        if (execArgs.Contains(Execution::Args::Type::AdminSettingEnable) && execArgs.Contains(Execution::Args::Type::AdminSettingDisable))
        {
            throw CommandException(Resource::String::TooManyAdminSettingArgumentsError(s_ArgName_EnableAndDisable));
        }

        if (execArgs.Contains(Execution::Args::Type::AdminSettingEnable) && AdminSetting::Unknown == StringToAdminSetting(execArgs.GetArg(Execution::Args::Type::AdminSettingEnable)))
        {
            throw CommandException(Resource::String::InvalidArgumentValueError(s_ArgumentName_Enable, s_ArgValue_EnableAndDisable_LocalManifestFiles));
        }

        if (execArgs.Contains(Execution::Args::Type::AdminSettingDisable) && AdminSetting::Unknown == StringToAdminSetting(execArgs.GetArg(Execution::Args::Type::AdminSettingDisable)))
        {
            throw CommandException(Resource::String::InvalidArgumentValueError(s_ArgumentName_Disable, s_ArgValue_EnableAndDisable_LocalManifestFiles));
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

    std::string SettingsExportCommand::HelpLink() const
    {
        return std::string{ s_SettingsCommand_HelpLink };
    }

    void SettingsExportCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::ExportSettings;
    }
}
