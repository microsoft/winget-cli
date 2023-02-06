// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallCommand.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/UpdateFlow.h"
#include "Workflows/MultiQueryFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Utility::literals;

namespace AppInstaller::CLI
{
    std::vector<Argument> InstallCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::MultiQuery),
            Argument::ForType(Args::Type::Manifest),
            Argument::ForType(Args::Type::Id),
            Argument::ForType(Args::Type::Name),
            Argument::ForType(Args::Type::Moniker),
            Argument::ForType(Args::Type::Version),
            Argument::ForType(Args::Type::Channel),
            Argument::ForType(Args::Type::Source),
            Argument{ Args::Type::InstallScope, Resource::String::InstallScopeDescription, ArgumentType::Standard, Argument::Visibility::Help },
            Argument::ForType(Args::Type::InstallArchitecture),
            Argument::ForType(Args::Type::Exact),
            Argument::ForType(Args::Type::Interactive),
            Argument::ForType(Args::Type::Silent),
            Argument::ForType(Args::Type::Locale),
            Argument::ForType(Args::Type::Log),
            Argument::ForType(Args::Type::CustomSwitches),
            Argument::ForType(Args::Type::Override),
            Argument::ForType(Args::Type::InstallLocation),
            Argument::ForType(Args::Type::HashOverride),
            Argument::ForType(Args::Type::IgnoreLocalArchiveMalwareScan),
            Argument::ForType(Args::Type::DependencySource),
            Argument::ForType(Args::Type::AcceptPackageAgreements),
            Argument::ForType(Args::Type::NoUpgrade),
            Argument::ForType(Args::Type::CustomHeader),
            Argument::ForType(Args::Type::AcceptSourceAgreements),
            Argument::ForType(Args::Type::Rename),
            Argument::ForType(Args::Type::UninstallPrevious),
            Argument::ForType(Args::Type::Force),
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

    void InstallCommand::Complete(Context& context, Args::Type valueType) const
    {
        switch (valueType)
        {
        case Args::Type::MultiQuery:
        case Args::Type::Manifest:
        case Args::Type::Id:
        case Args::Type::Name:
        case Args::Type::Moniker:
        case Args::Type::Version:
        case Args::Type::Channel:
        case Args::Type::Source:
            context <<
                Workflow::CompleteWithSingleSemanticsForValue(valueType);
            break;
        case Args::Type::InstallArchitecture:
        case Args::Type::Locale:
            // May well move to CompleteWithSingleSemanticsForValue,
            // but for now output nothing.
            context <<
                Workflow::CompleteWithEmptySet;
            break;
        case Args::Type::Log:
        case Args::Type::InstallLocation:
            // Intentionally output nothing to allow pass through to filesystem.
            break;
        }
    }

    Utility::LocIndView InstallCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-install"_liv;
    }

    void InstallCommand::ValidateArgumentsInternal(Args& execArgs) const
    {
        Argument::ValidateCommonArguments(execArgs);
    }

    void InstallCommand::ExecuteInternal(Context& context) const
    {
        context.SetFlags(ContextFlag::ShowSearchResultsOnPartialFailure);

        if (context.Args.Contains(Execution::Args::Type::Manifest))
        {
            context <<
                Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
                Workflow::GetManifestFromArg <<
                Workflow::SelectInstaller <<
                Workflow::EnsureApplicableInstaller <<
                Workflow::InstallSinglePackage;
        }
        else
        {
            context <<
                Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
                Workflow::OpenSource();

            if (!context.Args.Contains(Execution::Args::Type::Force))
            {
                context <<
                    Workflow::OpenCompositeSource(Repository::PredefinedSource::Installed, false, Repository::CompositeSearchBehavior::AvailablePackages);
            }

            if (context.Args.Contains(Execution::Args::Type::MultiQuery))
            {
                context <<
                    Workflow::GetMultiSearchRequests <<
                    Workflow::SearchSubContextsForSingle() <<
                    Workflow::ReportExecutionStage(Workflow::ExecutionStage::Execution) <<
                    Workflow::InstallMultiplePackages(
                        Resource::String::InstallAndUpgradeCommandsReportDependencies,
                        APPINSTALLER_CLI_ERROR_MULTIPLE_INSTALL_FAILED);
            }
            else
            {
                context << Workflow::InstallOrUpgradeSinglePackage(false);
            }
        }
    }
}
