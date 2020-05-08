// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SearchCommand.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace AppInstaller::CLI::Execution;
    using namespace std::string_view_literals;

    std::vector<Argument> SearchCommand::GetArguments() const
    {
        return {
            Argument::ForType(Execution::Args::Type::Query),
            Argument::ForType(Execution::Args::Type::Id),
            Argument::ForType(Execution::Args::Type::Name),
            Argument::ForType(Execution::Args::Type::Moniker),
            Argument::ForType(Execution::Args::Type::Tag),
            Argument::ForType(Execution::Args::Type::Command),
            Argument::ForType(Execution::Args::Type::Source),
            Argument::ForType(Execution::Args::Type::Count),
            Argument::ForType(Execution::Args::Type::Exact),
        };
    }

    std::string SearchCommand::ShortDescription() const
    {
        return Resources::GetInstance().ResolveWingetString(L"SearchCommandDescription");
    }

    std::string SearchCommand::GetLongDescription() const
    {
        return Resources::GetInstance().ResolveWingetString(L"SearchCommandDescription").c_str();
    }

    std::string SearchCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-search";
    }

    void SearchCommand::ExecuteInternal(Context& context) const
    {
        context <<
            Workflow::OpenSource <<
            Workflow::SearchSource <<
            Workflow::EnsureMatchesFromSearchResult <<
            Workflow::ReportSearchResult;
    }
}
