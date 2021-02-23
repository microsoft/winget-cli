// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallCommand.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::CLI::Workflow;

namespace AppInstaller::CLI
{
    std::vector<Argument> InstallCommand::GetArguments() const
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
        };
    }

    Resource::LocString InstallCommand::ShortDescription() const
    {
        return { Resource::String::InstallCommandShortDescription };
    }

    Resource::LocString InstallCommand::LongDescription() const
    {
        return { Resource::String::InstallCommandLongDescription };
    }

    void InstallCommand::Complete(Execution::Context& context, Execution::Args::Type valueType) const
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
        case Execution::Args::Type::InstallLocation:
            // Intentionally output nothing to allow pass through to filesystem.
            break;
        }
    }

    std::string InstallCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-install";
    }

    void InstallCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
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

    void InstallCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
            Workflow::GetManifest <<
            Workflow::InstallPackageVersion;
    }
}
