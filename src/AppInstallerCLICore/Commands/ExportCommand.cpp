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
    using namespace AppInstaller::CLI::Workflow;
    using namespace std::string_view_literals;

    std::vector<Argument> ExportCommand::GetArguments() const
    {
        return {
            Argument{ Execution::Args::Type::OutputFile, Resource::String::OutputFileArgumentDescription, ArgumentType::Positional, true },
            Argument{ Execution::Args::Type::Source, Resource::String::ExportSourceArgumentDescription, ArgumentType::Standard },
            Argument{ Execution::Args::Type::IncludeVersions, Resource::String::ExportIncludeVersionsArgumentDescription, ArgumentType::Flag },
            Argument::ForType(Execution::Args::Type::AcceptSourceAgreements),
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

    Utility::LocIndView ExportCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-export"_liv;
    }

    void ExportCommand::ExecuteInternal(Execution::Context& context) const
    {
        context.SetFlags(Execution::ContextFlag::TreatSourceFailuresAsWarning);

        context <<
            Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
            Workflow::OpenSource() <<
            Workflow::OpenCompositeSource(Repository::PredefinedSource::Installed) <<
            Workflow::SearchSourceForMany <<
            Workflow::HandleSearchResultFailures <<
            Workflow::EnsureMatchesFromSearchResult(OperationType::Export) <<
            Workflow::SelectVersionsToExport <<
            Workflow::ReportExecutionStage(ExecutionStage::Execution) <<
            Workflow::WriteImportFile;
    }
}
