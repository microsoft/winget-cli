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
            Argument{ s_ShowCommand_ArgName_Query, Execution::Args::Type::Query, LOCME("The query used to search for an app"), ArgumentType::Positional, true },
            Argument{ s_ShowCommand_ArgName_Id, Execution::Args::Type::Id, LOCME("The id of the application to show info"), ArgumentType::Standard },
            Argument{ s_ShowCommand_ArgName_Name, Execution::Args::Type::Name, LOCME("If specified, filter the results by name"), ArgumentType::Standard },
            Argument{ s_ShowCommand_ArgName_Moniker, Execution::Args::Type::Moniker, LOCME("If specified, filter the results by app moniker"), ArgumentType::Standard },
            Argument{ s_ShowCommand_ArgName_Version, Execution::Args::Type::Version, LOCME("If specified, use the specified version. Default is the latest version"), ArgumentType::Standard },
            Argument{ s_ShowCommand_ArgName_Channel, Execution::Args::Type::Channel, LOCME("If specified, use the specified channel. Default is general audience"), ArgumentType::Standard },
            Argument{ s_ShowCommand_ArgName_Source, Execution::Args::Type::Source, LOCME("If specified, find app using the specified source. Default is all source"), ArgumentType::Standard },
            Argument{ s_ShowCommand_ArgName_Exact, Execution::Args::Type::Exact, LOCME("If specified, find app using exact match"), ArgumentType::Flag },
            Argument{ s_ShowCommand_ArgName_ListVersions, Execution::Args::Type::ListVersions, LOCME("If specified, only show available versions of the app"), ArgumentType::Flag },
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

    void ShowCommand::ExecuteInternal(Execution::Context& context) const
    {
        ShowFlow appShowInfo{ context };

        appShowInfo.Execute();
    }
}
