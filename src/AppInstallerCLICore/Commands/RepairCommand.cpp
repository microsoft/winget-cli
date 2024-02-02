// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RepairCommand.h"
#include "Workflows/RepairFlow.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/InstallFlow.h"

namespace AppInstaller::CLI
{
    using namespace AppInstaller::CLI::Execution;
    using namespace AppInstaller::CLI::Workflow;

    std::vector<Argument> RepairCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::Manifest),                         // -m
            Argument::ForType(Args::Type::Id),                               // -id
            Argument::ForType(Args::Type::Name),                             // -n
            Argument::ForType(Args::Type::Moniker),                          // -mn
            Argument::ForType(Args::Type::Version),                          // -v
            Argument::ForType(Args::Type::InstallArchitecture),              // -arch
            Argument::ForType(Args::Type::Source),                           // -s
            Argument::ForType(Args::Type::Interactive),                      // -i
            Argument::ForType(Args::Type::Silent),                           // -h
            Argument::ForType(Args::Type::Log),                              // -o
            Argument::ForType(Args::Type::IgnoreLocalArchiveMalwareScan),    // -ignore-local-archive-malware-scan
            Argument::ForType(Args::Type::AcceptSourceAgreements),           // -accept-source-agreements
            Argument::ForType(Args::Type::OpenLogs),                         // -logs
            Argument::ForType(Args::Type::Override),                         // -locale : TODO: Determine if this is needed ?
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
        if (valueType == Execution::Args::Type::Manifest ||
            valueType == Execution::Args::Type::Log)
        {
            // Intentionally output nothing to allow pass through to filesystem.
            return;
        }

        switch (valueType)
        {
        case Execution::Args::Type::Id:
        case Execution::Args::Type::Name:
        case Execution::Args::Type::Moniker:
        case Execution::Args::Type::Version:
        case Execution::Args::Type::Channel:
        case Execution::Args::Type::Source:
            context <<
                Workflow::CompleteWithSingleSemanticsForValueUsingExistingSource(valueType);
            break;
        }
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
        context.SetFlags(Execution::ContextFlag::InstallerExecutionUseRepair);

        context <<
            Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
            Workflow::OpenSource() <<
            Workflow::OpenCompositeSource(Repository::PredefinedSource::Installed);

        if (context.Args.Contains(Args::Type::Manifest))
        {
            context <<
                Workflow::GetManifestFromArg <<
                Workflow::ReportManifestIdentity <<
                Workflow::SearchSourceUsingManifest <<
                Workflow::EnsureOneMatchFromSearchResult(OperationType::Repair) <<
                Workflow::GetInstalledPackageVersion <<
                Workflow::SelectInstaller <<
                Workflow::EnsureApplicableInstaller <<
                Workflow::RepairSinglePackage(OperationType::Repair);
        }
        else
        {
            if (!context.Args.Contains(Args::Type::MultiQuery))
            {
                context <<
                    Workflow::SearchSourceForSingle <<
                    Workflow::HandleSearchResultFailures <<
                    Workflow::EnsureOneMatchFromSearchResult(OperationType::Repair) <<
                    Workflow::GetInstalledPackageVersion <<
                    Workflow::SelectApplicablePackageVersion(true) <<
                    Workflow::EnsureApplicableInstaller <<
                    Workflow::ReportPackageIdentity <<
                    Workflow::RepairSinglePackage(OperationType::Repair);
            }
        }
    }
}
