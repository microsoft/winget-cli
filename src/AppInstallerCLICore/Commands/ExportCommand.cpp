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
            Argument{ "output", 'o', Execution::Args::Type::OutputFile, Resource::String::OutputFileArgumentDescription, ArgumentType::Positional, true },
            Argument{ "source", 's', Execution::Args::Type::Source, Resource::String::ExportSourceArgumentDescription, ArgumentType::Standard },
            Argument{ "include-versions", Argument::NoAlias, Execution::Args::Type::IncludeVersions, Resource::String::ExportIncludeVersionsArgumentDescription, ArgumentType::Flag },
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

        if (valueType == Execution::Args::Type::Source)
        {
            context << Workflow::CompleteSourceName;
            return;
        }
    }

    std::string ExportCommand::HelpLink() const
    {
        // TODO: point to correct location
        return "https://aka.ms/winget-command-export";
    }

    void ExportCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::ReportExecutionStage(Workflow::ExecutionStage::Discovery) <<
            Workflow::OpenSource <<
            Workflow::OpenCompositeSource(Repository::PredefinedSource::Installed) <<
            Workflow::SearchSourceForMany <<
            Workflow::EnsureMatchesFromSearchResult(true) <<
            Workflow::SelectVersionsToExport <<
            Workflow::ReportExecutionStage(Workflow::ExecutionStage::Execution) <<
            Workflow::WriteImportFile;
    }
}
