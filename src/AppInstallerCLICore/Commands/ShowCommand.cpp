// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ShowCommand.h"
#include "Localization.h"
#include "Workflows\ShowFlow.h"

namespace AppInstaller::CLI
{
    using namespace AppInstaller::Workflow;
    using namespace std::string_view_literals;

    std::vector<Argument> ShowCommand::GetArguments() const
    {
        return {
            Argument::ForType(Execution::Args::Type::Query),
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
        return LOCME("Shows info of the given application");
    }

    std::vector<std::string> ShowCommand::GetLongDescription() const
    {
        return {
            LOCME("Shows info of the given application"),
        };
    }

    void ShowCommand::ExecuteInternal(Execution::Context& context) const
    {
        ShowFlow appShowInfo{ context };

        appShowInfo.Execute();
    }
}
