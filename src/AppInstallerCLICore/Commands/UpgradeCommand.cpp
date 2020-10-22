// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "UpgradeCommand.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/UpdateFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::CLI::Workflow;

namespace AppInstaller::CLI
{
    std::vector<Argument> UpgradeCommand::GetArguments() const
    {
        return {
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
            Argument::ForType(Args::Type::Language),
            Argument::ForType(Args::Type::Log),
            Argument::ForType(Args::Type::Override),
            Argument::ForType(Args::Type::InstallLocation),
            Argument{ "force", Argument::NoAlias, Args::Type::Force, Resource::String::InstallForceArgumentDescription, ArgumentType::Flag },
            Argument{ "all", Argument::NoAlias, Args::Type::All, Resource::String::UpdateAllArgumentDescription, ArgumentType::Flag },
        };
    }

    Resource::LocString UpgradeCommand::ShortDescription() const
    {
        return { Resource::String::UpgradeCommandShortDescription };
    }

    Resource::LocString UpgradeCommand::LongDescription() const
    {
        return { Resource::String::UpgradeCommandLongDescription };
    }

    void UpgradeCommand::Complete(Execution::Context& context, Execution::Args::Type valueType) const
    {
        if (valueType == Execution::Args::Type::Manifest ||
            valueType == Execution::Args::Type::Log ||
            valueType == Execution::Args::Type::Override ||
            valueType == Execution::Args::Type::InstallLocation)
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
        case Execution::Args::Type::Language:
            // May well move to CompleteWithSingleSemanticsForValue,
            // but for now output nothing.
            context <<
                Workflow::CompleteWithEmptySet;
            break;
        }
    }

    std::string UpgradeCommand::HelpLink() const
    {
        // TODO: point to correct location
        return "https://aka.ms/winget-command-upgrade";
    }

    void UpgradeCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
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

    void UpgradeCommand::ExecuteInternal(Execution::Context& context) const
    {
        WI_SetFlag(context.GetFlags(), Execution::ContextFlag::InstallerExecutionUseUpdate);

        context <<
            Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
            OpenSource <<
            OpenCompositeSource(Repository::PredefinedSource::Installed);

        if (context.Args.Empty())
        {
            // Upgrade with no args list packages with updates available
            context <<
                Workflow::SearchSourceForMany <<
                Workflow::EnsureMatchesFromSearchResult <<
                Workflow::ReportListResult(true);
        }
        else if (context.Args.Contains(Execution::Args::Type::All))
        {
            // --all switch updates all packages found
            context <<
                SearchSourceForMany <<
                EnsureMatchesFromSearchResult <<
                UpdateAllApplicable;
        }
        else if (context.Args.Contains(Execution::Args::Type::Manifest))
        {
            // --manifest case where new manifest is provided
            context <<
                GetManifestFromArg <<
                ReportManifestIdentity <<
                SearchSourceUsingManifest <<
                EnsureOneMatchFromSearchResult <<
                GetInstalledPackageVersion <<
                EnsureUpdateVersionApplicable <<
                EnsureMinOSVersion <<
                SelectInstaller <<
                EnsureApplicableInstaller <<
                ShowInstallationDisclaimer <<
                Workflow::ReportExecutionStage(ExecutionStage::Download) <<
                DownloadInstaller <<
                Workflow::ReportExecutionStage(ExecutionStage::Execution) <<
                ExecuteInstaller <<
                Workflow::ReportExecutionStage(ExecutionStage::PostExecution) <<
                RemoveInstaller;
        }
        else
        {
            // The remaining case: search for single installed package to update
            context <<
                SearchSourceForSingle <<
                EnsureOneMatchFromSearchResult <<
                ReportSearchResultIdentity <<
                GetInstalledPackageVersion;

            if (context.Args.Contains(Execution::Args::Type::Version))
            {
                // If version specified, use the version and verify applicability
                context <<
                    GetManifestFromSearchResult <<
                    EnsureUpdateVersionApplicable <<
                    EnsureMinOSVersion <<
                    SelectInstaller <<
                    EnsureApplicableInstaller;
            }
            else
            {
                // iterate through available versions to find latest applicable update
                // This step also populates Manifest and Installer in context data
                context <<
                    SelectLatestApplicableUpdate(*(context.Get<Execution::Data::SearchResult>().Matches.at(0).Package));
            }

            context <<
                ShowInstallationDisclaimer <<
                Workflow::ReportExecutionStage(ExecutionStage::Download) <<
                DownloadInstaller <<
                Workflow::ReportExecutionStage(ExecutionStage::Execution) <<
                ExecuteInstaller <<
                Workflow::ReportExecutionStage(ExecutionStage::PostExecution) <<
                RemoveInstaller;
        }
    }
}
