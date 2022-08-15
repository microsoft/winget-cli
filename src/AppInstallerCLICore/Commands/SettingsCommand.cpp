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
}
