// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ImportCommand.h"
#include "Workflows/DownloadFlow.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/ImportExportFlow.h"
#include "Workflows/MultiQueryFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;

    std::vector<Argument> ImportCommand::GetArguments() const
    {
        return {
            Argument{ Execution::Args::Type::ImportFile, Resource::String::ImportFileArgumentDescription, ArgumentType::Positional, true },
            Argument{ Execution::Args::Type::IgnoreUnavailable, Resource::String::ImportIgnoreUnavailableArgumentDescription, ArgumentType::Flag },
            Argument{ Execution::Args::Type::IgnoreVersions, Resource::String::ImportIgnorePackageVersionsArgumentDescription, ArgumentType::Flag },
            Argument::ForType(Execution::Args::Type::NoUpgrade),
            Argument::ForType(Execution::Args::Type::AcceptPackageAgreements),
            Argument::ForType(Execution::Args::Type::AcceptSourceAgreements),
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

    Utility::LocIndView ImportCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-import"_liv;
    }

    void ImportCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::InitializeInstallerDownloadAuthenticatorsMap <<
            Workflow::ReportExecutionStage(Workflow::ExecutionStage::Discovery) <<
            Workflow::VerifyFile(Execution::Args::Type::ImportFile) <<
            Workflow::ReadImportFile <<
            Workflow::OpenSourcesForImport <<
            Workflow::OpenPredefinedSource(Repository::PredefinedSource::Installed) <<
            Workflow::GetSearchRequestsForImport <<
            Workflow::SearchSubContextsForSingle() <<
            Workflow::ReportExecutionStage(Workflow::ExecutionStage::Execution) <<
            Workflow::InstallImportedPackages;
    }
}
