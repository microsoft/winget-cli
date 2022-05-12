// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "UninstallCommand.h"
#include "Workflows/UninstallFlow.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

using AppInstaller::CLI::Execution::Args;
using AppInstaller::CLI::Workflow::ExecutionStage;

namespace AppInstaller::CLI
{
    std::vector<Argument> UninstallCommand::GetArguments() const
    {
        return
        {
            Argument::ForType(Args::Type::Query),
            Argument::ForType(Args::Type::Manifest),
            Argument::ForType(Args::Type::Id),
            Argument::ForType(Args::Type::Name),
            Argument::ForType(Args::Type::Moniker),
            Argument::ForType(Args::Type::ProductCode),
            Argument::ForType(Args::Type::Version),
            Argument::ForType(Args::Type::Channel),
            Argument::ForType(Args::Type::Source),
            Argument::ForType(Args::Type::Exact),
            Argument::ForType(Args::Type::Interactive),
            Argument::ForType(Args::Type::Silent),
            Argument::ForType(Args::Type::HashOverride), // TODO: Replace with proper name when behavior changes.
            Argument::ForType(Args::Type::Purge),
            Argument::ForType(Args::Type::Preserve),
            Argument::ForType(Args::Type::Log),
            Argument::ForType(Args::Type::CustomHeader),
            Argument::ForType(Args::Type::AcceptSourceAgreements),
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
        case Execution::Args::Type::Version:
        case Execution::Args::Type::Channel:
        case Execution::Args::Type::Source:
        case Execution::Args::Type::ProductCode:
            context <<
                Workflow::CompleteWithSingleSemanticsForValueUsingExistingSource(valueType);
            break;
        }
    }

    std::string UninstallCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-uninstall";
    }

    void UninstallCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        Argument::ValidatePackageSelectionArgumentSupplied(execArgs);

        if (execArgs.Contains(Execution::Args::Type::Manifest) &&
            (execArgs.Contains(Execution::Args::Type::Query) ||
             execArgs.Contains(Execution::Args::Type::Id) ||
             execArgs.Contains(Execution::Args::Type::Name) ||
             execArgs.Contains(Execution::Args::Type::Moniker) ||
             execArgs.Contains(Execution::Args::Type::ProductCode) ||
             execArgs.Contains(Execution::Args::Type::Version) ||
             execArgs.Contains(Execution::Args::Type::Channel) ||
             execArgs.Contains(Execution::Args::Type::Source) ||
             execArgs.Contains(Execution::Args::Type::Exact)))
        {
            throw CommandException(Resource::String::BothManifestAndSearchQueryProvided, "");
        }

        if (execArgs.Contains(Execution::Args::Type::Purge) && execArgs.Contains(Execution::Args::Type::Preserve))
        {
            throw CommandException(Resource::String::BothPurgeAndPreserveFlagsProvided, "");
        }
    }

    void UninstallCommand::ExecuteInternal(Execution::Context& context) const
    {
        context.SetFlags(Execution::ContextFlag::TreatSourceFailuresAsWarning);

        // open the sources where to search for the package
        context <<
            Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
            Workflow::OpenSource() <<
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
                Workflow::HandleSearchResultFailures <<
                Workflow::EnsureOneMatchFromSearchResult(true) <<
                Workflow::ReportPackageIdentity;
        }

        context <<
            Workflow::UninstallSinglePackage;
    }
}