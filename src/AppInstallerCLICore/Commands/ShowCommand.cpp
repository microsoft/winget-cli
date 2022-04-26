// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ShowCommand.h"
#include "Workflows/ShowFlow.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

using namespace AppInstaller::CLI::Execution;

namespace AppInstaller::CLI
{
    std::vector<Argument> ShowCommand::GetArguments() const
    {
        return {
            Argument::ForType(Execution::Args::Type::Query),
            // The manifest argument from Argument::ForType can be blocked by Group Policy but we don't want that here
            Argument{ "manifest", 'm', Execution::Args::Type::Manifest, Resource::String::ManifestArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help },
            Argument::ForType(Execution::Args::Type::Id),
            Argument::ForType(Execution::Args::Type::Name),
            Argument::ForType(Execution::Args::Type::Moniker),
            Argument::ForType(Execution::Args::Type::Version),
            Argument::ForType(Execution::Args::Type::Channel),
            Argument::ForType(Execution::Args::Type::Source),
            Argument::ForType(Execution::Args::Type::Exact),
            Argument::ForType(Execution::Args::Type::ListVersions),
            Argument::ForType(Execution::Args::Type::CustomHeader),
            Argument::ForType(Execution::Args::Type::AcceptSourceAgreements),
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

    void ShowCommand::Complete(Execution::Context& context, Execution::Args::Type valueType) const
    {
        context <<
            Workflow::CompleteWithSingleSemanticsForValue(valueType);
    }

    std::string ShowCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-show";
    }

    void ShowCommand::ValidateArgumentsInternal(Args& execArgs) const
    {
        Argument::ValidatePackageSelectionArgumentSupplied(execArgs);

        if (execArgs.Contains(Args::Type::Manifest) &&
            (execArgs.Contains(Args::Type::Query) ||
                execArgs.Contains(Args::Type::Id) ||
                execArgs.Contains(Args::Type::Name) ||
                execArgs.Contains(Args::Type::Moniker) ||
                execArgs.Contains(Args::Type::Version) ||
                execArgs.Contains(Args::Type::Channel) ||
                execArgs.Contains(Args::Type::Source) ||
                execArgs.Contains(Args::Type::Exact)))
        {
            throw CommandException(Resource::String::BothManifestAndSearchQueryProvided);
        }
    }

    void ShowCommand::ExecuteInternal(Execution::Context& context) const
    {
        context.SetFlags(Execution::ContextFlag::TreatSourceFailuresAsWarning);

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
                    Workflow::OpenSource() <<
                    Workflow::SearchSourceForSingle <<
                    Workflow::HandleSearchResultFailures <<
                    Workflow::EnsureOneMatchFromSearchResult(false) <<
                    Workflow::ReportPackageIdentity <<
                    Workflow::ShowAppVersions;
            }
        }
        else
        {
            context <<
                Workflow::GetManifest <<
                Workflow::ReportManifestIdentity <<
                Workflow::SelectInstaller <<
                Workflow::ShowManifestInfo;
        }
    }
}
