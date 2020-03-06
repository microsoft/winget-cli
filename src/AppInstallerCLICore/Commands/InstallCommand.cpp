// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallCommand.h"
#include "Localization.h"
#include "Manifest\Manifest.h"
#include "Workflows\InstallFlow.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Workflow;

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;

    constexpr std::string_view s_InstallCommand_ArgName_Query = "query"sv;
    constexpr std::string_view s_InstallCommand_ArgName_Manifest = "manifest"sv;
    constexpr std::string_view s_InstallCommand_ArgName_Id = "id"sv;
    constexpr std::string_view s_InstallCommand_ArgName_Name = "name"sv;
    constexpr std::string_view s_InstallCommand_ArgName_Moniker = "moniker"sv;
    constexpr std::string_view s_InstallCommand_ArgName_Version = "version"sv;
    constexpr std::string_view s_InstallCommand_ArgName_Channel = "channel"sv;
    constexpr std::string_view s_InstallCommand_ArgName_Source = "source"sv;
    constexpr std::string_view s_InstallCommand_ArgName_Exact = "exact"sv;
    constexpr std::string_view s_InstallCommand_ArgName_Interactive = "interactive"sv;
    constexpr std::string_view s_InstallCommand_ArgName_Silent = "silent"sv;
    constexpr std::string_view s_InstallCommand_ArgName_Language = "language"sv;
    constexpr std::string_view s_InstallCommand_ArgName_Log = "log"sv;
    constexpr std::string_view s_InstallCommand_ArgName_Override = "override"sv;

    std::vector<Argument> InstallCommand::GetArguments() const
    {
        return {
            Argument{ s_InstallCommand_ArgName_Query, ExecutionArgs::Type::Query, LOCME("The name of the application to install"), ArgumentType::Positional, false },
            Argument{ s_InstallCommand_ArgName_Manifest, ExecutionArgs::Type::Manifest, LOCME("The path to the manifest of the application to install"), ArgumentType::Standard, false },
            Argument{ s_InstallCommand_ArgName_Id, ExecutionArgs::Type::Id, LOCME("The id of the application to show info"), ArgumentType::Standard },
            Argument{ s_InstallCommand_ArgName_Name, ExecutionArgs::Type::Name, LOCME("If specified, filter the results by name"), ArgumentType::Standard },
            Argument{ s_InstallCommand_ArgName_Moniker, ExecutionArgs::Type::Moniker, LOCME("If specified, filter the results by app moniker"), ArgumentType::Standard },
            Argument{ s_InstallCommand_ArgName_Version, ExecutionArgs::Type::Version, LOCME("If specified, use the specified version. Default is the latest version"), ArgumentType::Standard },
            Argument{ s_InstallCommand_ArgName_Channel, ExecutionArgs::Type::Channel, LOCME("If specified, use the specified channel. Default is general audience"), ArgumentType::Standard },
            Argument{ s_InstallCommand_ArgName_Source, ExecutionArgs::Type::Source, LOCME("If specified, find app using the specified source. Default is all source"), ArgumentType::Standard },
            Argument{ s_InstallCommand_ArgName_Exact, ExecutionArgs::Type::Exact, LOCME("If specified, find app using exact match"), ArgumentType::Flag },
            Argument{ s_InstallCommand_ArgName_Interactive, ExecutionArgs::Type::Interactive, LOCME("The application installation is interactive. User input is needed."), ArgumentType::Flag, false },
            Argument{ s_InstallCommand_ArgName_Silent, ExecutionArgs::Type::Silent, LOCME("The application installation is silent."), ArgumentType::Flag, false },
            Argument{ s_InstallCommand_ArgName_Language, ExecutionArgs::Type::Language, LOCME("Preferred language if application installation supports multiple languages."), ArgumentType::Standard, false },
            Argument{ s_InstallCommand_ArgName_Log, ExecutionArgs::Type::Log, LOCME("Preferred log location if application installation supports custom log path."), ArgumentType::Standard, false },
            Argument{ s_InstallCommand_ArgName_Override, ExecutionArgs::Type::Override, LOCME("Override switches to be passed on to application installer."), ArgumentType::Standard, false },
        };
    }

    std::string InstallCommand::ShortDescription() const
    {
        return LOCME("Installs the given application");
    }

    std::vector<std::string> InstallCommand::GetLongDescription() const
    {
        return {
            LOCME("Installs the given application"),
        };
    }

    void InstallCommand::ExecuteInternal(ExecutionContext& context) const
    {
        InstallFlow appInstall(context);

        appInstall.Execute();
    }

    void InstallCommand::ValidateArguments(ExecutionArgs& execArgs) const
    {
        Command::ValidateArguments(execArgs);

        if (!execArgs.Contains(ExecutionArgs::Type::Query) && !execArgs.Contains(ExecutionArgs::Type::Manifest))
        {
            throw CommandException(LOCME("Required argument not provided"), s_InstallCommand_ArgName_Query);
        }

        if (execArgs.Contains(ExecutionArgs::Type::Silent) && execArgs.Contains(ExecutionArgs::Type::Interactive))
        {
            throw CommandException(LOCME("More than one install behavior argument provided"), s_InstallCommand_ArgName_Query);
        }
    }
}
