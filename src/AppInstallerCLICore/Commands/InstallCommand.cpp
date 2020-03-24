// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallCommand.h"
#include "Localization.h"
#include "Manifest\Manifest.h"
#include "Workflows\InstallFlow.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Workflow;

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;

    constexpr std::string_view s_InstallCommand_ArgName_QueryOrManifest = "query|manifest"sv;

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
            Argument::ForType(Execution::Args::Type::ListVersions),
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
        return LOCME("Installs the given application");
    }

    std::vector<std::string> InstallCommand::GetLongDescription() const
    {
        return {
            LOCME("Installs the given application"),
        };
    }

    void InstallCommand::ExecuteInternal(Execution::Context& context) const
    {
        InstallFlow appInstall(context);

        appInstall.Execute();
    }

    void InstallCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        Command::ValidateArguments(execArgs);

        // TODO: Maybe one day implement argument groups
        if (!execArgs.Contains(Execution::Args::Type::Query) && !execArgs.Contains(Execution::Args::Type::Manifest))
        {
            throw CommandException(LOCME("Required argument not provided"), s_InstallCommand_ArgName_QueryOrManifest);
        }

        if (execArgs.Contains(Execution::Args::Type::Silent) && execArgs.Contains(Execution::Args::Type::Interactive))
        {
            throw CommandException(LOCME("More than one install behavior argument provided"), s_InstallCommand_ArgName_QueryOrManifest);
        }
    }
}
