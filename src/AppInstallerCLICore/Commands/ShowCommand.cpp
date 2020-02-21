// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Common.h"
#include "ShowCommand.h"
#include "Localization.h"
#include "Workflows\ShowFlow.h"

namespace AppInstaller::CLI
{
    using namespace AppInstaller::Workflow;

    std::vector<Argument> ShowCommand::GetArguments() const
    {
        return {
            Argument{ ARG_QUERY, LOCME("The query used to search for an app"), ArgumentType::Positional, true },
            Argument{ ARG_ID, LOCME("The id of the application to show info"), ArgumentType::Standard },
            Argument{ ARG_NAME, LOCME("If specified, filter the results by name"), ArgumentType::Standard },
            Argument{ ARG_MONIKER, LOCME("If specified, filter the results by app moniker"), ArgumentType::Standard },
            Argument{ ARG_VERSION, LOCME("If specified, use the specified version. Default is the latest version"), ArgumentType::Standard },
            Argument{ ARG_CHANNEL, LOCME("If specified, use the specified channel. Default is general audience"), ArgumentType::Standard },
            Argument{ ARG_SOURCE, LOCME("If specified, find app using the specified source. Default is all source"), ArgumentType::Standard },
            Argument{ ARG_EXACT, LOCME("If specified, find app using exact match"), ArgumentType::Flag },
            Argument{ ARG_LISTVERSIONS, LOCME("If specified, only show available versions of the app"), ArgumentType::Flag },
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

    void ShowCommand::ExecuteInternal(Invocation& inv, std::ostream& out, std::istream& in) const
    {
        ShowFlow appShowInfo(inv, out, in);

        appShowInfo.Execute();
    }
}
