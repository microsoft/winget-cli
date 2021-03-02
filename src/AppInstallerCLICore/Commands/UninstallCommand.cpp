// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "UninstallCommand.h"
#include "Workflows/UninstallFlow.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

using AppInstaller::CLI::Execution::Args;
using AppInstaller::CLI::Workflow::ExecutionStage;

namespace AppInstaller::CLI
{
    std::vector<Argument> UninstallCommand::GetArguments() const
    {
        // TODO: determine exact arguments needed
        return
        {
            Argument::ForType(Args::Type::Query),
            Argument::ForType(Args::Type::Manifest),
            Argument::ForType(Args::Type::Id),
            Argument::ForType(Args::Type::Name),
            Argument::ForType(Args::Type::Moniker),
            Argument::ForType(Args::Type::Version),
            Argument::ForType(Args::Type::Channel),
            Argument::ForType(Args::Type::Source),
            Argument::ForType(Args::Type::Exact),
            Argument::ForType(Args::Type::Interactive),
            Argument::ForType(Args::Type::Silent),
            Argument::ForType(Args::Type::Log),
        };
    }

    Resource::LocString UninstallCommand::ShortDescription() const
    {
        return { Resource::String::UninstallCommandShortDescription };
    }

    Resource::LocString UninstallCommand::LongDescription() const
    {
        return { Resource::String::UninstallCommandLongDescription };
    }

    void UninstallCommand::Complete(Execution::Context& context, Execution::Args::Type valueType) const
    {
        if (valueType == Execution::Args::Type::Manifest ||
            valueType == Execution::Args::Type::Log)
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
        case Execution::Args::Type::Version:
        case Execution::Args::Type::Channel:
        case Execution::Args::Type::Source:
            context <<
                Workflow::CompleteWithSingleSemanticsForValueUsingExistingSource(valueType);
            break;
        }
    }

    std::string UninstallCommand::HelpLink() const
    {
        // TODO: point to correct location
        return "https://aka.ms/winget-command-uninstall";
    }

    void UninstallCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        if (execArgs.Contains(Execution::Args::Type::Manifest) &&
            (execArgs.Contains(Execution::Args::Type::Query) ||
             execArgs.Contains(Execution::Args::Type::Id) ||
             execArgs.Contains(Execution::Args::Type::Name) ||
             execArgs.Contains(Execution::Args::Type::Moniker) ||
             execArgs.Contains(Execution::Args::Type::Version) ||
             execArgs.Contains(Execution::Args::Type::Channel) ||
             execArgs.Contains(Execution::Args::Type::Source) ||
             execArgs.Contains(Execution::Args::Type::Exact)))
        {
            throw CommandException(Resource::String::BothManifestAndSearchQueryProvided, "");
        }
    }

    void UninstallCommand::ExecuteInternal(Execution::Context& context) const
    {
        // open the sources where to search for the package
        context <<
            Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
            Workflow::OpenSource <<
            Workflow::OpenCompositeSource(Repository::PredefinedSource::Installed);

        // find the uninstaller
        if (context.Args.Contains(Execution::Args::Type::Manifest))
        {
            // --manifest case where new manifest is provided
            context <<
                Workflow::GetManifestFromArg <<
                Workflow::ReportManifestIdentity <<
                Workflow::SearchSourceUsingManifest <<
                Workflow::EnsureOneMatchFromSearchResult(true);
        }
        else
        {
            // search for a single package to uninstall
            context <<
                Workflow::SearchSourceForSingle <<
                Workflow::EnsureOneMatchFromSearchResult(true) <<
                Workflow::ReportPackageIdentity;
        }

        context <<
            Workflow::GetInstalledPackageVersion <<
            Workflow::GetUninstallInfo <<
            Workflow::ReportExecutionStage(ExecutionStage::Execution) <<
            Workflow::ExecuteUninstaller <<
            Workflow::ReportExecutionStage(ExecutionStage::PostExecution);
    }
}