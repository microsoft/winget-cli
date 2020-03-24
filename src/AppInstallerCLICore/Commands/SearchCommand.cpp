// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SearchCommand.h"
#include "Localization.h"
#include "Workflows/SearchFlow.h"

namespace AppInstaller::CLI
{
    using namespace AppInstaller::Workflow;
    using namespace std::string_view_literals;

    constexpr std::string_view s_SearchCommand_ArgName_Query = "query"sv;
    constexpr std::string_view s_SearchCommand_ArgName_Id = "id"sv;
    constexpr std::string_view s_SearchCommand_ArgName_Name = "name"sv;
    constexpr std::string_view s_SearchCommand_ArgName_Moniker = "moniker"sv;
    constexpr std::string_view s_SearchCommand_ArgName_Tag = "tag"sv;
    constexpr std::string_view s_SearchCommand_ArgName_Command = "command"sv;
    constexpr std::string_view s_SearchCommand_ArgName_Source = "source"sv;
    constexpr std::string_view s_SearchCommand_ArgName_Count = "count"sv;
    constexpr std::string_view s_SearchCommand_ArgName_Exact = "exact"sv;

    std::vector<Argument> SearchCommand::GetArguments() const
    {
        return {
            Argument{ s_SearchCommand_ArgName_Query, Execution::Args::Type::Query, LOCME("The query used to search for an app"), ArgumentType::Positional, false },
            Argument{ s_SearchCommand_ArgName_Id, Execution::Args::Type::Id, LOCME("If specified, filter the results by id"), ArgumentType::Standard },
            Argument{ s_SearchCommand_ArgName_Name, Execution::Args::Type::Name, LOCME("If specified, filter the results by name"), ArgumentType::Standard },
            Argument{ s_SearchCommand_ArgName_Moniker, Execution::Args::Type::Moniker, LOCME("If specified, filter the results by app moniker"), ArgumentType::Standard },
            Argument{ s_SearchCommand_ArgName_Tag, Execution::Args::Type::Tag, LOCME("If specified, filter the results by tag"), ArgumentType::Standard },
            Argument{ s_SearchCommand_ArgName_Command, Execution::Args::Type::Command, LOCME("If specified, filter the results by command"), ArgumentType::Standard },
            Argument{ s_SearchCommand_ArgName_Source, Execution::Args::Type::Source, LOCME("If specified, find app using the specified source. Default is all source"), ArgumentType::Standard },
            Argument{ s_SearchCommand_ArgName_Count, Execution::Args::Type::Count, LOCME("If specified, find app and show only up to specified number of results."), ArgumentType::Standard },
            Argument{ s_SearchCommand_ArgName_Exact, Execution::Args::Type::Exact, LOCME("If specified, find app using exact match"), ArgumentType::Flag },
        };
    }

    std::string SearchCommand::ShortDescription() const
    {
        return LOCME("Find and show basic info of apps");
    }

    std::vector<std::string> SearchCommand::GetLongDescription() const
    {
        return {
            LOCME("Find and show basic info of apps"),
        };
    }

    void SearchCommand::ExecuteInternal(Execution::Context& context) const
    {
        SearchFlow appSearch{ context };

        appSearch.Execute();
    }
}
