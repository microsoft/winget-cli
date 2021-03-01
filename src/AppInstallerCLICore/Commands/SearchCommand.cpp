// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SearchCommand.h"
#include "Workflows/CompletionFlow.h"
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

    Resource::LocString SearchCommand::ShortDescription() const
    {
        return { Resource::String::SearchCommandShortDescription };
    }

    Resource::LocString SearchCommand::LongDescription() const
    {
        return { Resource::String::SearchCommandLongDescription };
    }

    void SearchCommand::Complete(Execution::Context& context, Execution::Args::Type valueType) const
    {
        switch (valueType)
        {
        case Execution::Args::Type::Query:
            context <<
                Workflow::OpenSource <<
                Workflow::RequireCompletionWordNonEmpty <<
                Workflow::SearchSourceForManyCompletion <<
                Workflow::CompleteWithMatchedField;
            break;
        case Execution::Args::Type::Id:
        case Execution::Args::Type::Name:
        case Execution::Args::Type::Moniker:
        case Execution::Args::Type::Tag:
        case Execution::Args::Type::Command:
        case Execution::Args::Type::Source:
            context <<
                Workflow::CompleteWithSingleSemanticsForValue(valueType);
            break;
        }
    }

    std::string SearchCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-search";
    }

    void SearchCommand::ExecuteInternal(Context& context) const
    {
        context <<
            Workflow::OpenSource <<
            Workflow::SearchSourceForMany <<
            Workflow::EnsureMatchesFromSearchResult(false) <<
            Workflow::ReportSearchResult;
    }
}
