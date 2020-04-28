// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ShowCommand.h"
#include "Localization.h"
#include "Workflows/ShowFlow.h"
#include "Workflows/WorkflowBase.h"

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
        };
    }

    std::string ShowCommand::ShortDescription() const
    {
        return LOCME("Shows info about an application");
    }

    std::string ShowCommand::GetLongDescription() const
    {
        return LOCME("Shows information on a specific application.");
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
                    Workflow::SearchSource <<
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
        }
    }
}
