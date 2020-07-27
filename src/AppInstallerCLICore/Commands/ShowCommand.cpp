// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ShowCommand.h"
#include "Workflows/ShowFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"
#include "Workflows/GetRequestedUrlFromManifest.h"

namespace AppInstaller::CLI
{
    std::vector<Argument> ShowCommand::GetArguments() const
    {
        return {
            Argument::ForType(Execution::Args::Type::Query),
            Argument::ForType(Execution::Args::Type::Manifest),
            Argument::ForType(Execution::Args::Type::Id),
            Argument::ForType(Execution::Args::Type::Name),
            Argument::ForType(Execution::Args::Type::Moniker),
            Argument::ForType(Execution::Args::Type::Version),
            Argument::ForType(Execution::Args::Type::Channel),
            Argument::ForType(Execution::Args::Type::Source),
            Argument::ForType(Execution::Args::Type::Exact),
            Argument::ForType(Execution::Args::Type::ListVersions),
            Argument::ForType(Execution::Args::Type::Homepage),
            Argument::ForType(Execution::Args::Type::License),
        };
    }

    Resource::LocString ShowCommand::ShortDescription() const
    {
        return { Resource::String::ShowCommandShortDescription };
    }

    Resource::LocString ShowCommand::LongDescription() const
    {
        return { Resource::String::ShowCommandLongDescription };
    }

    std::string ShowCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-show";
    }

    void ShowCommand::ExecuteInternal(Execution::Context& context) const
    {
        if (context.Args.Contains(Execution::Args::Type::ListVersions))
        {
            if (context.Args.Contains(Execution::Args::Type::Manifest))
            {
                context <<
                    Workflow::GetManifestFromArg <<
                    Workflow::ReportManifestIdentity <<
                    Workflow::ShowManifestVersion;
            }
            else
            {
                context <<
                    Workflow::OpenSource <<
                    Workflow::SearchSourceForSingle <<
                    Workflow::EnsureOneMatchFromSearchResult <<
                    Workflow::ReportSearchResultIdentity <<
                    Workflow::ShowAppVersions;
            }
        }
        else
        {
            context <<
                Workflow::GetManifest <<
                Workflow::SelectInstaller <<
                Workflow::ShowManifestInfo;

            if (context.Args.Contains(Execution::Args::Type::Homepage) ||
                context.Args.Contains(Execution::Args::Type::License))
            {
                context <<
                    Workflow::EnsureFeatureEnabled(Settings::ExperimentalFeature::Feature::BrowseOrOpenRemoteUrl) <<
                    Workflow::GetRequestedUrlFromManifest <<
                    Workflow::OpenUrlInDefaultBrowser;
            }
        }
    }
}
