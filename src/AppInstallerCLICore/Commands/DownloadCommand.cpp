// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DownloadCommand.h"
#include "Workflows/DownloadFlow.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/PromptFlow.h"
#include "Resources.h"
#include <AppInstallerRuntime.h>

namespace AppInstaller::CLI
{
    using namespace AppInstaller::CLI::Execution;
    using namespace AppInstaller::CLI::Workflow;
    using namespace AppInstaller::Utility::literals;

    std::vector<Argument> DownloadCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::Query),
            Argument::ForType(Args::Type::DownloadDirectory),
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
            Argument::ForType(Args::Type::Locale),
            Argument::ForType(Args::Type::HashOverride),
            Argument::ForType(Args::Type::SkipDependencies),
            Argument::ForType(Execution::Args::Type::AcceptPackageAgreements),
            Argument::ForType(Execution::Args::Type::AcceptSourceAgreements),
        };
    }

    Resource::LocString DownloadCommand::ShortDescription() const
    {
        return { Resource::String::DownloadCommandShortDescription };
    }

    Resource::LocString DownloadCommand::LongDescription() const
    {
        return { Resource::String::DownloadCommandLongDescription };
    }

    Utility::LocIndView DownloadCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-download"_liv;
    }

    void DownloadCommand::ValidateArgumentsInternal(Args& execArgs) const
    {
        Argument::ValidateCommonArguments(execArgs);
    }

    void DownloadCommand::ExecuteInternal(Context& context) const
    {
        context.SetFlags(AppInstaller::CLI::Execution::ContextFlag::InstallerDownloadOnly);

        if (context.Args.Contains(Execution::Args::Type::Manifest))
        {
            context <<
                Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
                Workflow::GetManifestFromArg;
        }
        else
        {
            context <<
                Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
                Workflow::OpenSource() <<
                Workflow::SearchSourceForSingle <<
                Workflow::HandleSearchResultFailures <<
                Workflow::EnsureOneMatchFromSearchResult(OperationType::Download) <<
                Workflow::GetManifestFromPackage(false);
        }

        context <<
            Workflow::SetDownloadDirectory <<
            Workflow::SelectInstaller <<
            Workflow::EnsureApplicableInstaller <<
            Workflow::ReportIdentityAndInstallationDisclaimer <<
            Workflow::ShowPromptsForSinglePackage(/* ensureAcceptance */ true) <<
            Workflow::DownloadPackageDependencies <<
            Workflow::DownloadInstaller;
    }
}
