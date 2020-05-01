// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallCommand.h"
#include "Localization.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::CLI::Workflow;

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;

    constexpr std::string_view s_InstallCommand_ArgName_SilentAndInteractive = "silent|interactive"sv;

    std::vector<Argument> InstallCommand::GetArguments() const
    {
        return {
            Argument::ForType(Execution::Args::Type::Query),
            Argument::ForType(Execution::Args::Type::Manifest),
            Argument::ForType(Execution::Args::Type::Id),
            Argument::ForType(Execution::Args::Type::Name),
            Argument::ForType(Execution::Args::Type::Moniker),
            Argument::ForType(Execution::Args::Type::Version),
            Argument::ForType(Execution::Args::Type::Channel),
            Argument::ForType(Execution::Args::Type::Source),
            Argument::ForType(Execution::Args::Type::Exact),
            Argument::ForType(Execution::Args::Type::Interactive),
            Argument::ForType(Execution::Args::Type::Silent),
            Argument::ForType(Execution::Args::Type::Language),
            Argument::ForType(Execution::Args::Type::Log),
            Argument::ForType(Execution::Args::Type::Override),
            Argument::ForType(Execution::Args::Type::InstallLocation),
        };
    }

    std::string InstallCommand::ShortDescription() const
    {
        return Resources::GetInstance().ResolveWingetString(L"InstallCommandDescription");
    }

    std::string InstallCommand::GetLongDescription() const
    {
        return Resources::GetInstance().ResolveWingetString(L"InstallCommandDescription");
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
            Workflow::VerifyInstallerHash <<
            Workflow::ExecuteInstaller;
    }

    void InstallCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        if (execArgs.Contains(Execution::Args::Type::Silent) && execArgs.Contains(Execution::Args::Type::Interactive))
        {
            throw CommandException(Resources::GetInstance().ResolveWingetString(L"TooManyBehaviorsError"), s_InstallCommand_ArgName_SilentAndInteractive);
        }
    }
}
