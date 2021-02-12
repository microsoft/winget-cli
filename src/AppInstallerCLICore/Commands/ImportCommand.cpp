// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ImportCommand.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/ImportExportFlow.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;

    std::vector<Argument> ImportCommand::GetArguments() const
    {
        return {
            Argument{ "import-file", 'i', Execution::Args::Type::ImportFile, Resource::String::ImportFileArgumentDescription, ArgumentType::Positional, true },
            Argument{ "ignore-unavailable", Argument::NoAlias, Execution::Args::Type::IgnoreUnavailable, Resource::String::ImportIgnoreUnavailableArgumentDescription, ArgumentType::Flag },
            Argument{ "ignore-versions", Argument::NoAlias, Execution::Args::Type::IgnoreVersions, Resource::String::ImportIgnorePackageVersionsArgumentDescription, ArgumentType::Flag },
        };
    }

    Resource::LocString ImportCommand::ShortDescription() const
    {
        return { Resource::String::ImportCommandShortDescription };
    }

    Resource::LocString ImportCommand::LongDescription() const
    {
        return { Resource::String::ImportCommandLongDescription };
    }

    std::string ImportCommand::HelpLink() const
    {
        // TODO: point to correct location
        return "https://aka.ms/winget-command-import";
    }

    void ImportCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::ReportExecutionStage(Workflow::ExecutionStage::Discovery) <<
            Workflow::VerifyFile(Execution::Args::Type::ImportFile) <<
            Workflow::ReadImportFile <<
            Workflow::OpenSourcesForImport <<
            Workflow::OpenPredefinedSource(Repository::PredefinedSource::Installed) <<
            Workflow::SearchPackagesForImport <<
            Workflow::ReportExecutionStage(Workflow::ExecutionStage::Execution) <<
            Workflow::InstallMultiple;
    }
}
