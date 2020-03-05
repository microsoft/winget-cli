// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ShowCommand.h"
#include "Localization.h"
#include "Workflows\ShowFlow.h"

namespace AppInstaller::CLI
{
    using namespace AppInstaller::Workflow;
    using namespace std::string_view_literals;

    constexpr std::string_view s_ShowCommand_ArgName_Query = "query"sv;
    constexpr std::string_view s_ShowCommand_ArgName_Id = "id"sv;
    constexpr std::string_view s_ShowCommand_ArgName_Name = "name"sv;
    constexpr std::string_view s_ShowCommand_ArgName_Moniker = "moniker"sv;
    constexpr std::string_view s_ShowCommand_ArgName_Version = "version"sv;
    constexpr std::string_view s_ShowCommand_ArgName_Channel = "channel"sv;
    constexpr std::string_view s_ShowCommand_ArgName_Source = "source"sv;
    constexpr std::string_view s_ShowCommand_ArgName_Exact = "exact"sv;
    constexpr std::string_view s_ShowCommand_ArgName_ListVersions = "listversions"sv;

    std::vector<Argument> ShowCommand::GetArguments() const
    {
        return {
            Argument{ s_ShowCommand_ArgName_Query, LOCME("The query used to search for an app"), ArgumentType::Positional, true },
            Argument{ s_ShowCommand_ArgName_Id, LOCME("The id of the application to show info"), ArgumentType::Standard },
            Argument{ s_ShowCommand_ArgName_Name, LOCME("If specified, filter the results by name"), ArgumentType::Standard },
            Argument{ s_ShowCommand_ArgName_Moniker, LOCME("If specified, filter the results by app moniker"), ArgumentType::Standard },
            Argument{ s_ShowCommand_ArgName_Version, LOCME("If specified, use the specified version. Default is the latest version"), ArgumentType::Standard },
            Argument{ s_ShowCommand_ArgName_Channel, LOCME("If specified, use the specified channel. Default is general audience"), ArgumentType::Standard },
            Argument{ s_ShowCommand_ArgName_Source, LOCME("If specified, find app using the specified source. Default is all source"), ArgumentType::Standard },
            Argument{ s_ShowCommand_ArgName_Exact, LOCME("If specified, find app using exact match"), ArgumentType::Flag },
            Argument{ s_ShowCommand_ArgName_ListVersions, LOCME("If specified, only show available versions of the app"), ArgumentType::Flag },
        };
    }

    std::string ShowCommand::ShortDescription() const
    {
        return LOCME("Shows info of the given application");
    }

    std::vector<std::string> ShowCommand::GetLongDescription() const
    {
        return {
            LOCME("Shows info of the given application"),
        };
    }

    void ShowCommand::ExecuteInternal(ExecutionContext& context) const
    {
        ShowFlow appShowInfo{ context };

        appShowInfo.Execute();
    }

    ExecutionArgs::ExecutionArgType ShowCommand::GetExecutionArgType(std::string_view argName) const
    {
        if (argName == s_ShowCommand_ArgName_Query)
        {
            return ExecutionArgs::ExecutionArgType::Query;
        }
        else if (argName == s_ShowCommand_ArgName_Id)
        {
            return ExecutionArgs::ExecutionArgType::Id;
        }
        else if (argName == s_ShowCommand_ArgName_Name)
        {
            return ExecutionArgs::ExecutionArgType::Name;
        }
        else if (argName == s_ShowCommand_ArgName_Moniker)
        {
            return ExecutionArgs::ExecutionArgType::Moniker;
        }
        else if (argName == s_ShowCommand_ArgName_Version)
        {
            return ExecutionArgs::ExecutionArgType::Version;
        }
        else if (argName == s_ShowCommand_ArgName_Channel)
        {
            return ExecutionArgs::ExecutionArgType::Channel;
        }
        else if (argName == s_ShowCommand_ArgName_Source)
        {
            return ExecutionArgs::ExecutionArgType::Source;
        }
        else if (argName == s_ShowCommand_ArgName_Exact)
        {
            return ExecutionArgs::ExecutionArgType::Exact;
        }
        else if (argName == s_ShowCommand_ArgName_ListVersions)
        {
            return ExecutionArgs::ExecutionArgType::ListVersions;
        }
        else
        {
            THROW_HR(E_UNEXPECTED);
        }
    }
}
