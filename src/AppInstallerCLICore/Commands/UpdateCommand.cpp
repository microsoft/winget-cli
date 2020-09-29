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

    void UpdateCommand::Complete(Execution::Context& context, Execution::Args::Type valueType) const
    {
        switch (valueType)
        {
        case Execution::Args::Type::Query:
        case Execution::Args::Type::Manifest:
        case Execution::Args::Type::Id:
        case Execution::Args::Type::Name:
        case Execution::Args::Type::Moniker:
        case Execution::Args::Type::Version:
        case Execution::Args::Type::Channel:
        case Execution::Args::Type::Source:
            context <<
                Workflow::CompleteWithSingleSemanticsForValue(valueType);
            break;
        case Execution::Args::Type::Language:
            // May well move to CompleteWithSingleSemanticsForValue,
            // but for now output nothing.
            context <<
                Workflow::CompleteWithEmptySet;
            break;
        case Execution::Args::Type::Log:
            // Intentionally output nothing to allow pass through to filesystem.
            break;
        }
    }

    std::string UpdateCommand::HelpLink() const
    {
        // TODO: point to correct location
        return "https://aka.ms/winget-command-update";
    }

    void UpdateCommand::ExecuteInternal(Execution::Context& context) const
    {
        context.Add<Execution::Data::CommandType>(CommandType::Update);

        if (context.Args.Contains(Execution::Args::Type::All))
        {
            context <<
                GetCompositeSourceFromInstalledAndAvailable <<
                SearchSourceForMany <<
                EnsureMatchesFromSearchResult;

            const auto& matches = context.Get<Execution::Data::SearchResult>().Matches;
            bool updateAllHasFailure = false;
            for (const auto& match : matches)
            {
                std::thread tryOneUpdate([&]
                    {
                        // We want to do best effort to update all applicable updates regardless on previous update failure
                        context.Resume();
                        context.Reporter.Info() << std::endl;
                        context <<
                            SelectLatestApplicableUpdate(*(match.Package)) <<
                            ShowInstallationDisclaimer <<
                            DownloadInstaller <<
                            ExecuteInstaller <<
                            RemoveInstaller;
                    });

                tryOneUpdate.join();

                if (context.GetTerminationHR() != S_OK &&
                    context.GetTerminationHR() != APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE)
                {
                    updateAllHasFailure = true;
                }
            }

            AICLI_TERMINATE_CONTEXT(updateAllHasFailure ? APPINSTALLER_CLI_ERROR_UPDATE_ALL_HAS_FAILURE : S_OK);
        }
        else
        {
            context <<
                Workflow::GetUpdateManifestAndInstaller <<
                Workflow::ShowInstallationDisclaimer <<
                Workflow::DownloadInstaller <<
                Workflow::ExecuteInstaller <<
                Workflow::RemoveInstaller;
        }
    }
}
