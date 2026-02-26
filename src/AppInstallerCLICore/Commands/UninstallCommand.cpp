// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "UninstallCommand.h"
#include "Workflows/UninstallFlow.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Workflows/MultiQueryFlow.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using AppInstaller::CLI::Execution::Args;
    using namespace AppInstaller::CLI::Workflow;

    std::vector<Argument> UninstallCommand::GetArguments() const
    {
        return
        {
            Argument::ForType(Args::Type::MultiQuery),
            Argument::ForType(Args::Type::Manifest),
            Argument::ForType(Args::Type::Id),
            Argument::ForType(Args::Type::Name),
            Argument::ForType(Args::Type::Moniker),
            Argument::ForType(Args::Type::ProductCode),
            Argument::ForType(Args::Type::TargetVersion),
            Argument::ForType(Args::Type::AllVersions),
            Argument::ForType(Args::Type::Channel),
            Argument::ForType(Args::Type::Source),
            Argument::ForType(Args::Type::Exact),
            Argument{ Args::Type::InstallScope, Resource::String::InstalledScopeArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help },
            Argument::ForType(Args::Type::Interactive),
            Argument::ForType(Args::Type::Silent),
            Argument::ForType(Args::Type::Force),
            Argument::ForType(Args::Type::Purge),
            Argument::ForType(Args::Type::Preserve),
            Argument::ForType(Args::Type::Log),
            Argument::ForType(Args::Type::CustomHeader),
            Argument::ForType(Args::Type::AuthenticationMode),
            Argument::ForType(Args::Type::AuthenticationAccount),
            Argument::ForType(Args::Type::AcceptSourceAgreements),
        };
    }

    Resource::LocString UninstallCommand::ShortDescription() const
    {
        return { Resource::String::UninstallCommandShortDescription };
    }

    Resource::LocString UninstallCommand::LongDescription() const
    {
        return { Resource::String::UninstallCommandLongDescription };
    }

    void UninstallCommand::Complete(Execution::Context& context, Execution::Args::Type valueType) const
    {
        if (valueType == Execution::Args::Type::Manifest ||
            valueType == Execution::Args::Type::Log)
        {
            // Intentionally output nothing to allow pass through to filesystem.
            return;
        }

        context <<
            Workflow::OpenSource() <<
            Workflow::OpenCompositeSource(Repository::PredefinedSource::Installed);

        switch (valueType)
        {
        case Execution::Args::Type::MultiQuery:
            context <<
                Workflow::RequireCompletionWordNonEmpty <<
                Workflow::SearchSourceForManyCompletion <<
                Workflow::CompleteWithMatchedField;
            break;
        case Execution::Args::Type::Id:
        case Execution::Args::Type::Name:
        case Execution::Args::Type::Moniker:
        case Execution::Args::Type::TargetVersion:
        case Execution::Args::Type::Channel:
        case Execution::Args::Type::Source:
        case Execution::Args::Type::ProductCode:
            context <<
                Workflow::CompleteWithSingleSemanticsForValueUsingExistingSource(valueType);
            break;
        }
    }

    Utility::LocIndView UninstallCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-uninstall"_liv;
    }

    void UninstallCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        Argument::ValidateCommonArguments(execArgs);
    }

    void UninstallCommand::ExecuteInternal(Execution::Context& context) const
    {
        context.SetFlags(Execution::ContextFlag::TreatSourceFailuresAsWarning);

        // open the sources where to search for the package
        context <<
            Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
            Workflow::OpenSource() <<
            Workflow::OpenCompositeSource(Workflow::DetermineInstalledSource(context));

        // find the uninstaller
        if (context.Args.Contains(Execution::Args::Type::Manifest))
        {
            // --manifest case where new manifest is provided
            context <<
                Workflow::GetManifestFromArg <<
                Workflow::SearchSourceUsingManifest <<
                Workflow::EnsureOneMatchFromSearchResult(OperationType::Uninstall) <<
                Workflow::UninstallSinglePackage;
        }
        else
        {
            // search for specific packages to uninstall
            if (!context.Args.Contains(Execution::Args::Type::MultiQuery))
            {
                context <<
                    Workflow::SearchSourceForSingle <<
                    Workflow::HandleSearchResultFailures <<
                    Workflow::EnsureOneMatchFromSearchResult(OperationType::Uninstall) <<
                    Workflow::UninstallSinglePackage;
            }
            else
            {
                context <<
                    Workflow::GetMultiSearchRequests <<
                    Workflow::SearchSubContextsForSingle(OperationType::Uninstall) <<
                    Workflow::UninstallMultiplePackages;
            }
        }
    }
}