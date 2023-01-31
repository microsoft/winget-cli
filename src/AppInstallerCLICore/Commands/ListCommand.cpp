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
            Argument{ Execution::Args::Type::InstallScope, Resource::String::InstalledScopeArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help },
            Argument::ForType(Execution::Args::Type::CustomHeader),
            Argument::ForType(Execution::Args::Type::AcceptSourceAgreements),
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
            Workflow::OpenSource() <<
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

    Utility::LocIndView ListCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-list"_liv;
    }

    void ListCommand::ExecuteInternal(Execution::Context& context) const
    {
        context.SetFlags(Execution::ContextFlag::TreatSourceFailuresAsWarning);

        context <<
            Workflow::OpenSource() <<
            Workflow::OpenCompositeSource(Workflow::DetermineInstalledSource(context)) <<
            Workflow::SearchSourceForMany <<
            Workflow::HandleSearchResultFailures <<
            Workflow::EnsureMatchesFromSearchResult(true) <<
            Workflow::ReportListResult();
    }
}
