// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "UpgradeCommand.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/UpdateFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Workflows/DependenciesFlow.h"
#include "Resources.h"

using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::CLI::Workflow;

namespace AppInstaller::CLI
{
    namespace
    {
        // Determines whether there are any arguments only used in search queries,
        // as opposed to listing available upgrades
        bool HasSearchQueryArguments(Execution::Args& execArgs)
        {
            // Note that this does not include Manifest (no search) or source related args (used for listing)
            return execArgs.Contains(Args::Type::Query) ||
                execArgs.Contains(Args::Type::Id) ||
                execArgs.Contains(Args::Type::Name) ||
                execArgs.Contains(Args::Type::Moniker) ||
                execArgs.Contains(Args::Type::Version) ||
                execArgs.Contains(Args::Type::Channel) ||
                execArgs.Contains(Args::Type::Exact);
        }

        // Determines whether there are any arguments only used when upgrading a single package,
        // as opposed to upgrading multiple packages or listing all available upgrades
        bool HasArgumentsForSinglePackage(Execution::Args& execArgs)
        {
            return HasSearchQueryArguments(execArgs) ||
                execArgs.Contains(Args::Type::Manifest);
        }

        // Determines whether there are any arguments only used when dealing with multiple packages,
        // either for upgrading or for listing available upgrades.
        bool HasArgumentsForMultiplePackages(Execution::Args& execArgs)
        {
            return execArgs.Contains(Args::Type::All);  
        }

        // Determines whether there are any arguments only used as options during an upgrade,
        // as opposed to listing available upgrades or selecting the packages.
        bool HasArgumentsForInstallOptions(Execution::Args& execArgs)
        {
            return execArgs.Contains(Args::Type::Interactive) ||
                execArgs.Contains(Args::Type::Silent) ||
                execArgs.Contains(Args::Type::Log) ||
                execArgs.Contains(Args::Type::Override) ||
                execArgs.Contains(Args::Type::InstallLocation) ||
                execArgs.Contains(Args::Type::HashOverride) ||
                execArgs.Contains(Args::Type::AcceptPackageAgreements);
        }

        // Determines whether there are any arguments related to the source.
        bool HasArgumentsForSource(Execution::Args& execArgs)
        {
            return execArgs.Contains(Args::Type::Source) ||
                execArgs.Contains(Args::Type::CustomHeader) ||
                execArgs.Contains(Args::Type::AcceptSourceAgreements);
        }

        // Determines whether we should list available upgrades, instead
        // of performing an upgrade
        bool ShouldListUpgrade(Execution::Args& execArgs)
        {
            // Valid arguments for list are only those related to the sources and which packages to include.
            // Instead of checking for them, we check that there aren't any other arguments present.
            return !execArgs.Contains(Args::Type::All) &&
                !HasArgumentsForSinglePackage(execArgs) &&
                !HasArgumentsForInstallOptions(execArgs);
        }
    }

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
            Argument::ForType(Args::Type::Purge),
            Argument::ForType(Args::Type::Log),
            Argument::ForType(Args::Type::Override),
            Argument::ForType(Args::Type::InstallLocation),
            Argument::ForType(Args::Type::HashOverride),
            Argument::ForType(Args::Type::AcceptPackageAgreements),
            Argument::ForType(Args::Type::AcceptSourceAgreements),
            Argument::ForType(Execution::Args::Type::CustomHeader),
            Argument{ "all", Argument::NoAlias, Args::Type::All, Resource::String::UpdateAllArgumentDescription, ArgumentType::Flag },
            Argument{ "include-unknown", Argument::NoAlias, Args::Type::IncludeUnknown, Resource::String::IncludeUnknownArgumentDescription, ArgumentType::Flag }
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
            OpenSource() <<
            OpenCompositeSource(Repository::PredefinedSource::Installed);

        switch (valueType)
        {
        case Execution::Args::Type::Query:
            context <<
                RequireCompletionWordNonEmpty <<
                SearchSourceForManyCompletion <<
                CompleteWithMatchedField;
            break;
        case Execution::Args::Type::Id:
        case Execution::Args::Type::Name:
        case Execution::Args::Type::Moniker:
        case Execution::Args::Type::Version:
        case Execution::Args::Type::Channel:
        case Execution::Args::Type::Source:
            context <<
                CompleteWithSingleSemanticsForValueUsingExistingSource(valueType);
            break;
        }
    }

    std::string UpgradeCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-upgrade";
    }

    void UpgradeCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        if (execArgs.Contains(Execution::Args::Type::Manifest) &&
            (HasSearchQueryArguments(execArgs) ||
                HasArgumentsForMultiplePackages(execArgs) ||
                HasArgumentsForSource(execArgs)))
        {
            throw CommandException(Resource::String::BothManifestAndSearchQueryProvided);
        }


        else if (!ShouldListUpgrade(execArgs) 
                && !HasSearchQueryArguments(execArgs) 
                && (execArgs.Contains(Args::Type::Log) ||
                    execArgs.Contains(Args::Type::Override) ||
                    execArgs.Contains(Args::Type::InstallLocation) ||
                    execArgs.Contains(Args::Type::HashOverride) ||
                    execArgs.Contains(Args::Type::AcceptPackageAgreements)))
        {
            throw CommandException(Resource::String::InvalidArgumentWithoutQueryError);
        }
    }

    void UpgradeCommand::ExecuteInternal(Execution::Context& context) const
    {
        context.SetFlags(Execution::ContextFlag::InstallerExecutionUseUpdate);

        // Only allow for source failures when doing a list of available upgrades.
        // We have to set it now to allow for source open failures to also just warn.
        if (ShouldListUpgrade(context.Args))
        {
            context.SetFlags(Execution::ContextFlag::TreatSourceFailuresAsWarning);
        }

        context <<
            Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
            Workflow::OpenSource() <<
            Workflow::OpenCompositeSource(Repository::PredefinedSource::Installed);

        if (ShouldListUpgrade(context.Args))
        {
            // Upgrade with no args list packages with updates available
            context <<
                SearchSourceForMany <<
                HandleSearchResultFailures <<
                EnsureMatchesFromSearchResult(true) <<
                ReportListResult(true);
        }
        else if (context.Args.Contains(Execution::Args::Type::All))
        {
            // --all switch updates all packages found
            context <<
                SearchSourceForMany <<
                HandleSearchResultFailures <<
                EnsureMatchesFromSearchResult(true) <<
                ReportListResult(true) <<
                UpdateAllApplicable;
        }
        else if (context.Args.Contains(Execution::Args::Type::Manifest))
        {
            // --manifest case where new manifest is provided
            context <<
                GetManifestFromArg <<
                SearchSourceUsingManifest <<
                EnsureOneMatchFromSearchResult(true) <<
                GetInstalledPackageVersion <<
                EnsureUpdateVersionApplicable <<
                SelectInstaller <<
                EnsureApplicableInstaller <<
                InstallSinglePackage;
        }
        else
        {
            // The remaining case: search for single installed package to update
            context <<
                SearchSourceForSingle <<
                HandleSearchResultFailures <<
                EnsureOneMatchFromSearchResult(true) <<
                GetInstalledPackageVersion;

            if (context.Args.Contains(Execution::Args::Type::Version))
            {
                // If version specified, use the version and verify applicability
                context <<
                    GetManifestFromPackage <<
                    EnsureUpdateVersionApplicable <<
                    SelectInstaller <<
                    EnsureApplicableInstaller;
            }
            else
            {
                // iterate through available versions to find latest applicable update
                // This step also populates Manifest and Installer in context data
                context << SelectLatestApplicableUpdate(true);
            }

            context << InstallSinglePackage;
        }
    }
}
