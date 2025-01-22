// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DownloadCommand.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/DownloadFlow.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/PromptFlow.h"
#include "Resources.h"
#include <AppInstallerRuntime.h>
#include <winget/ManifestCommon.h>

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
            Argument::ForType(Args::Type::InstallerArchitecture),
            Argument::ForType(Args::Type::InstallerType),
            Argument::ForType(Args::Type::Exact),
            Argument::ForType(Args::Type::Locale),
            Argument::ForType(Args::Type::HashOverride),
            Argument::ForType(Args::Type::SkipDependencies),
            Argument::ForType(Args::Type::CustomHeader),
            Argument::ForType(Args::Type::AuthenticationMode),
            Argument::ForType(Args::Type::AuthenticationAccount),
            Argument::ForType(Args::Type::AcceptPackageAgreements),
            Argument::ForType(Args::Type::AcceptSourceAgreements),
            Argument::ForType(Args::Type::SkipMicrosoftStorePackageLicense),
            Argument::ForType(Args::Type::Platform),
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

    void DownloadCommand::Complete(Context& context, Args::Type valueType) const
    {
        switch (valueType)
        {
        case Args::Type::Query:
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
        case Args::Type::InstallerArchitecture:
        case Args::Type::Locale:
            // May well move to CompleteWithSingleSemanticsForValue,
            // but for now output nothing.
            context <<
                Workflow::CompleteWithEmptySet;
            break;
        case Args::Type::Log:
        case Args::Type::DownloadDirectory:
            // Intentionally output nothing to allow pass through to filesystem.
            break;
        }
    }

    Utility::LocIndView DownloadCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-download"_liv;
    }

    void DownloadCommand::ValidateArgumentsInternal(Args& execArgs) const
    {
        Argument::ValidateCommonArguments(execArgs);

        if (execArgs.Contains(Execution::Args::Type::Platform))
        {
            Manifest::PlatformEnum selectedPlatform = Manifest::ConvertToPlatformEnumForMSStoreDownload(execArgs.GetArg(Execution::Args::Type::Platform));
            if (selectedPlatform == Manifest::PlatformEnum::Unknown)
            {
                auto validOptions = Utility::Join(", "_liv, std::vector<Utility::LocIndString>{
                    "Windows.Universal"_lis, "Windows.Desktop"_lis, "Windows.IoT"_lis, "Windows.Team"_lis, "Windows.Holographic"_lis
                });
                throw CommandException(Resource::String::InvalidArgumentValueError(Argument::ForType(Execution::Args::Type::Platform).Name(), validOptions));
            }
        }
    }

    void DownloadCommand::ExecuteInternal(Context& context) const
    {
        context.SetFlags(AppInstaller::CLI::Execution::ContextFlag::InstallerDownloadOnly);

        context << Workflow::InitializeInstallerDownloadAuthenticatorsMap;

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
