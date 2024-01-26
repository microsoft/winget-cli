// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RepairCommand.h"
#include "Workflows/RepairFlow.h"

namespace AppInstaller::CLI
{
    using namespace AppInstaller::CLI::Execution;
    using namespace AppInstaller::CLI::Workflow;

    std::vector<Argument> RepairCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::Manifest),     // -m
            Argument::ForType(Args::Type::Id),           // -id
            Argument::ForType(Args::Type::Name),         // -n
            Argument::ForType(Args::Type::Moniker),      // -mn
            Argument::ForType(Args::Type::Version),      // -v
            Argument::ForType(Args::Type::Channel),      // -c
            Argument::ForType(Args::Type::Source),       // -s
            Argument::ForType(Args::Type::Interactive),  // -i
            Argument::ForType(Args::Type::Silent),       // -q
            Argument::ForType(Args::Type::Log),          // -o
        };
    }

    Resource::LocString RepairCommand::ShortDescription() const
    {
        return { Resource::String::RepairCommandShortDescription };
    }

    Resource::LocString RepairCommand::LongDescription() const
    {
        return { Resource::String::RepairCommandLongDescription };
    }

    void RepairCommand::Complete(Execution::Context& context, Execution::Args::Type valueType) const
    {
        UNREFERENCED_PARAMETER(context);
        UNREFERENCED_PARAMETER(valueType);
        // TODO: implement
    }

    Utility::LocIndView RepairCommand::HelpLink() const
    {
        // TODO: point to the right place
        return "https://aka.ms/winget-command-repair"_liv;
    }

    void RepairCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        Argument::ValidateCommonArguments(execArgs);
    }

    void RepairCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
            Workflow::OpenSource() <<
            Workflow::OpenCompositeSource(Repository::PredefinedSource::Installed);

        if (context.Args.Contains(Args::Type::Manifest))
        {
            context <<
                GetManifestFromArg <<
                SearchSourceUsingManifest <<
                Workflow::EnsureOneMatchFromSearchResult(OperationType::Repair) <<
                Workflow::RepairSinglePackage(OperationType::Repair);
        }
        else
        {
            if (!context.Args.Contains(Args::Type::MultiQuery))
            {
                context <<
                    Workflow::SearchSourceForSingle <<
                    Workflow::EnsureOneMatchFromSearchResult(OperationType::Repair) <<
                    Workflow::RepairSinglePackage(OperationType::Repair);
            }
        }
    }
}
