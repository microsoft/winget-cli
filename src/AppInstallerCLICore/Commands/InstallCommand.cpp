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
            Argument{ s_InstallCommand_ArgName_Query, LOCME("The name of the application to install"), ArgumentType::Positional, false },
            Argument{ s_InstallCommand_ArgName_Manifest, LOCME("The path to the manifest of the application to install"), ArgumentType::Standard, false },
            Argument{ s_InstallCommand_ArgName_Id, LOCME("The id of the application to show info"), ArgumentType::Standard },
            Argument{ s_InstallCommand_ArgName_Name, LOCME("If specified, filter the results by name"), ArgumentType::Standard },
            Argument{ s_InstallCommand_ArgName_Moniker, LOCME("If specified, filter the results by app moniker"), ArgumentType::Standard },
            Argument{ s_InstallCommand_ArgName_Version, LOCME("If specified, use the specified version. Default is the latest version"), ArgumentType::Standard },
            Argument{ s_InstallCommand_ArgName_Channel, LOCME("If specified, use the specified channel. Default is general audience"), ArgumentType::Standard },
            Argument{ s_InstallCommand_ArgName_Source, LOCME("If specified, find app using the specified source. Default is all source"), ArgumentType::Standard },
            Argument{ s_InstallCommand_ArgName_Exact, LOCME("If specified, find app using exact match"), ArgumentType::Flag },
            Argument{ s_InstallCommand_ArgName_Interactive, LOCME("The application installation is interactive. User input is needed."), ArgumentType::Flag, false },
            Argument{ s_InstallCommand_ArgName_Silent, LOCME("The application installation is silent."), ArgumentType::Flag, false },
            Argument{ s_InstallCommand_ArgName_Language, LOCME("Preferred language if application installation supports multiple languages."), ArgumentType::Standard, false },
            Argument{ s_InstallCommand_ArgName_Log, LOCME("Preferred log location if application installation supports custom log path."), ArgumentType::Standard, false },
            Argument{ s_InstallCommand_ArgName_Override, LOCME("Override switches to be passed on to application installer."), ArgumentType::Standard, false },
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

    void InstallCommand::ValidateArguments(Invocation& inv) const
    {
        Command::ValidateArguments(inv);

        if (!inv.Contains(s_InstallCommand_ArgName_Query) && !inv.Contains(s_InstallCommand_ArgName_Manifest))
        {
            throw CommandException(LOCME("Required argument not provided"), s_InstallCommand_ArgName_Query);
        }

        if (inv.Contains(s_InstallCommand_ArgName_Silent) && inv.Contains(s_InstallCommand_ArgName_Interactive))
        {
            throw CommandException(LOCME("More than one install behavior argument provided"), s_InstallCommand_ArgName_Query);
        }
    }

    ExecutionArgs::ExecutionArgType InstallCommand::GetExecutionArgType(std::string_view argName) const
    {
        if (argName == s_InstallCommand_ArgName_Query)
        {
            return ExecutionArgs::ExecutionArgType::Query;
        }
        else if (argName == s_InstallCommand_ArgName_Manifest)
        {
            return ExecutionArgs::ExecutionArgType::Manifest;
        }
        else if (argName == s_InstallCommand_ArgName_Id)
        {
            return ExecutionArgs::ExecutionArgType::Id;
        }
        else if (argName == s_InstallCommand_ArgName_Name)
        {
            return ExecutionArgs::ExecutionArgType::Name;
        }
        else if (argName == s_InstallCommand_ArgName_Moniker)
        {
            return ExecutionArgs::ExecutionArgType::Moniker;
        }
        else if (argName == s_InstallCommand_ArgName_Version)
        {
            return ExecutionArgs::ExecutionArgType::Version;
        }
        else if (argName == s_InstallCommand_ArgName_Channel)
        {
            return ExecutionArgs::ExecutionArgType::Channel;
        }
        else if (argName == s_InstallCommand_ArgName_Source)
        {
            return ExecutionArgs::ExecutionArgType::Source;
        }
        else if (argName == s_InstallCommand_ArgName_Exact)
        {
            return ExecutionArgs::ExecutionArgType::Exact;
        }
        else if (argName == s_InstallCommand_ArgName_Interactive)
        {
            return ExecutionArgs::ExecutionArgType::Interactive;
        }
        else if (argName == s_InstallCommand_ArgName_Silent)
        {
            return ExecutionArgs::ExecutionArgType::Silent;
        }
        else if (argName == s_InstallCommand_ArgName_Language)
        {
            return ExecutionArgs::ExecutionArgType::Language;
        }
        else if (argName == s_InstallCommand_ArgName_Log)
        {
            return ExecutionArgs::ExecutionArgType::Log;
        }
        else if (argName == s_InstallCommand_ArgName_Override)
        {
            return ExecutionArgs::ExecutionArgType::Override;
        }
        else
        {
            THROW_HR(E_UNEXPECTED);
        }
    }
}
