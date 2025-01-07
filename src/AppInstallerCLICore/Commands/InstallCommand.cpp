// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerRuntime.h"
#include "CheckpointManager.h"
#include "InstallCommand.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/DownloadFlow.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/UpdateFlow.h"
#include "Workflows/MultiQueryFlow.h"
#include "Workflows/ResumeFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace AppInstaller::CLI::Execution;
    using namespace AppInstaller::CLI::Workflow;
    using namespace AppInstaller::Manifest;
    using namespace AppInstaller::Utility::literals;

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
            Argument::ForType(Args::Type::InstallerType),
            Argument::ForType(Args::Type::Exact),
            Argument::ForType(Args::Type::Interactive),
            Argument::ForType(Args::Type::Silent),
            Argument::ForType(Args::Type::Locale),
            Argument::ForType(Args::Type::Log),
            Argument::ForType(Args::Type::CustomSwitches),
            Argument::ForType(Args::Type::Override),
            Argument::ForType(Args::Type::InstallLocation),
            Argument::ForType(Args::Type::HashOverride),
            Argument::ForType(Args::Type::AllowReboot),
            Argument::ForType(Args::Type::SkipDependencies),
            Argument::ForType(Args::Type::IgnoreLocalArchiveMalwareScan),
            Argument::ForType(Args::Type::DependencySource),
            Argument::ForType(Args::Type::AcceptPackageAgreements),
            Argument::ForType(Args::Type::NoUpgrade),
            Argument::ForType(Args::Type::CustomHeader),
            Argument::ForType(Args::Type::AuthenticationMode),
            Argument::ForType(Args::Type::AuthenticationAccount),
            Argument::ForType(Args::Type::AcceptSourceAgreements),
            Argument::ForType(Args::Type::Rename),
            Argument::ForType(Args::Type::UninstallPrevious),
            Argument::ForType(Args::Type::Force),
            Argument{ Args::Type::IncludeUnknown, Resource::String::IncludeUnknownArgumentDescription, ArgumentType::Flag, Argument::Visibility::Hidden},
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

    void InstallCommand::Resume(Context& context) const
    {
        // TODO: Load context data from checkpoint for install command.
        ExecuteInternal(context);
    }

    void InstallCommand::ExecuteInternal(Context& context) const
    {
        context.SetFlags(ContextFlag::ShowSearchResultsOnPartialFailure);

        context << Workflow::InitializeInstallerDownloadAuthenticatorsMap;

        if (context.Args.Contains(Execution::Args::Type::Manifest))
        {
            context <<
                Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
                Workflow::GetManifestFromArg <<
                Workflow::SelectInstaller <<
                Workflow::EnsureApplicableInstaller <<
                Workflow::Checkpoint("PreInstallCheckpoint", {}) << // TODO: Capture context data
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
                    Workflow::OpenCompositeSource(Workflow::DetermineInstalledSource(context), false, Repository::CompositeSearchBehavior::AvailablePackages);
            }

            if (context.Args.Contains(Execution::Args::Type::MultiQuery))
            {
                bool skipDependencies = Settings::User().Get<Settings::Setting::InstallSkipDependencies>() || context.Args.Contains(Execution::Args::Type::SkipDependencies);
                context <<
                    Workflow::GetMultiSearchRequests <<
                    Workflow::SearchSubContextsForSingle() <<
                    Workflow::ReportExecutionStage(Workflow::ExecutionStage::Execution) <<
                    Workflow::ProcessMultiplePackages(
                        Resource::String::PackageRequiresDependencies,
                        APPINSTALLER_CLI_ERROR_MULTIPLE_INSTALL_FAILED,
                        {}, true, skipDependencies);
            }
            else
            {
                context <<
                    Workflow::Checkpoint("PreInstallCheckpoint", {}) << // TODO: Capture context data
                    Workflow::InstallOrUpgradeSinglePackage(OperationType::Install);
            }
        }
    }
}
