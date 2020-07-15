// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallCommand.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::CLI::Workflow;

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;

    constexpr std::string_view s_InstallCommand_ArgName_SilentAndInteractive = "silent|interactive"sv;

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

    std::string InstallCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-install";
    }

    void InstallCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::GetManifest <<
            Workflow::EnsureMinOSVersion <<
            Workflow::SelectInstaller <<
            Workflow::EnsureApplicableInstaller <<
            Workflow::ShowInstallationDisclaimer <<
            Workflow::DownloadInstaller <<
            Workflow::ExecuteInstaller <<
            Workflow::RemoveInstaller;
    }

    void InstallCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        if (execArgs.Contains(Execution::Args::Type::Silent) && execArgs.Contains(Execution::Args::Type::Interactive))
        {
            throw CommandException(Resource::String::TooManyBehaviorsError, s_InstallCommand_ArgName_SilentAndInteractive);
        }
    }
}
