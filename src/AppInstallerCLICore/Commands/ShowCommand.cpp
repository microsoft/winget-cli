// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ShowCommand.h"
#include "Localization.h"
#include "Workflows/ShowFlow.h"

namespace AppInstaller::CLI
{
    std::vector<Argument> ShowCommand::GetArguments() const
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
        };
    }

    std::string ShowCommand::ShortDescription() const
    {
        return LOCME("Shows info about an application");
    }

    std::string ShowCommand::GetLongDescription() const
    {
        return LOCME("Shows information on a specific application.");
    }

    void ShowCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::
    }
}
