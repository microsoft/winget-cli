// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ExportCommand.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/ImportExportFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;

    std::vector<Argument> ExportCommand::GetArguments() const
    {
        return {
            Argument::ForType(Execution::Args::Type::OutputFile),
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

    Resource::LocString ExportCommand::ShortDescription() const
    {
        return { Resource::String::ExportCommandShortDescription };
    }

    Resource::LocString ExportCommand::LongDescription() const
    {
        return { Resource::String::ExportCommandLongDescription };
    }

    void ExportCommand::Complete(Execution::Context& context, Execution::Args::Type valueType) const
    {
        if (valueType == Execution::Args::Type::OutputFile)
        {
            // Intentionally output nothing to allow pass through to filesystem.
            return;
        }

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

    std::string ExportCommand::HelpLink() const
    {
        // TODO: point to correct location
        return "https://aka.ms/winget-command-export";
    }

    void ExportCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        if (execArgs.Contains(Execution::Args::Type::Manifest) &&
            (execArgs.Contains(Execution::Args::Type::Query) ||
                execArgs.Contains(Execution::Args::Type::Id) ||
                execArgs.Contains(Execution::Args::Type::Name) ||
                execArgs.Contains(Execution::Args::Type::Moniker) ||
                execArgs.Contains(Execution::Args::Type::Version) ||
                execArgs.Contains(Execution::Args::Type::Channel) ||
                execArgs.Contains(Execution::Args::Type::Source) ||
                execArgs.Contains(Execution::Args::Type::Exact) ||
                execArgs.Contains(Execution::Args::Type::All)))
        {
            throw CommandException(Resource::String::BothManifestAndSearchQueryProvided, "");
        }
    }

    void ExportCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::OpenSource <<
            Workflow::OpenCompositeSource(Repository::PredefinedSource::Installed) <<
            Workflow::SearchSourceForMany <<
            Workflow::EnsureMatchesFromSearchResult(true) <<
            Workflow::Export;
    }
}
