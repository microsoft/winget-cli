// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "UpdateCommand.h"
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
    std::vector<Argument> UpdateCommand::GetArguments() const
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

    Resource::LocString UpdateCommand::ShortDescription() const
    {
        return { Resource::String::UpdateCommandShortDescription };
    }

    Resource::LocString UpdateCommand::LongDescription() const
    {
        return { Resource::String::UpdateCommandLongDescription };
    }

    void UpdateCommand::Complete(Execution::Context&, Execution::Args::Type) const
    {
        // TODO: Should be done similar to list completion
    }

    std::string UpdateCommand::HelpLink() const
    {
        // TODO: point to correct location
        return "https://aka.ms/winget-command-upgrade";
    }

    void UpdateCommand::ExecuteInternal(Execution::Context& context) const
    {
        context.Get<Execution::Data::ContextFlag>() |= Execution::ContextFlag::InstallerExecutionUseUpdate;

        context <<
            OpenSource <<
            GetCompositeSourceFromInstalledAndAvailable;

        if (context.Args.Empty())
        {
            // Upgrade with no args list packages with updates available
            // TODO: go to list filtered to packages with update available
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
                DownloadInstaller <<
                ExecuteInstaller <<
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
                    DownloadInstaller <<
                    ExecuteInstaller <<
                    RemoveInstaller;
        }
    }
}
