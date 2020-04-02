// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SearchCommand.h"
#include "Localization.h"
#include "Workflows/WorkflowBase.h"

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
        return LOCME("Find and show basic info of apps");
    }

    std::string SearchCommand::GetLongDescription() const
    {
        return LOCME("Find and show basic info of apps");
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
