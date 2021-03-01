// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ListCommand.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;

    std::vector<Argument> ListCommand::GetArguments() const
    {
        return {
            Argument::ForType(Execution::Args::Type::Query),
            Argument::ForType(Execution::Args::Type::Id),
            Argument::ForType(Execution::Args::Type::Name),
            Argument::ForType(Execution::Args::Type::Moniker),
            Argument::ForType(Execution::Args::Type::Source),
            Argument::ForType(Execution::Args::Type::Tag),
            Argument::ForType(Execution::Args::Type::Command),
            Argument::ForType(Execution::Args::Type::Count),
            Argument::ForType(Execution::Args::Type::Exact),
        };
    }

    Resource::LocString ListCommand::ShortDescription() const
    {
        return { Resource::String::ListCommandShortDescription };
    }

    Resource::LocString ListCommand::LongDescription() const
    {
        return { Resource::String::ListCommandLongDescription };
    }

    void ListCommand::Complete(Execution::Context& context, Execution::Args::Type valueType) const
    {
        context <<
            Workflow::OpenSource <<
            Workflow::OpenCompositeSource(Repository::PredefinedSource::Installed);

        switch (valueType)
        {
        case Execution::Args::Type::Query:
            context <<
                Workflow::RequireCompletionWordNonEmpty <<
                Workflow::SearchSourceForManyCompletion <<
                Workflow::CompleteWithMatchedField;
            break;
        case Execution::Args::Type::Id:
        case Execution::Args::Type::Name:
        case Execution::Args::Type::Moniker:
        case Execution::Args::Type::Source:
        case Execution::Args::Type::Tag:
        case Execution::Args::Type::Command:
            context <<
                Workflow::CompleteWithSingleSemanticsForValueUsingExistingSource(valueType);
            break;
        }
    }

    std::string ListCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-list";
    }

    void ListCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::OpenSource <<
            Workflow::OpenCompositeSource(Repository::PredefinedSource::Installed) <<
            Workflow::SearchSourceForMany <<
            Workflow::EnsureMatchesFromSearchResult(true) <<
            Workflow::ReportListResult();
    }
}
