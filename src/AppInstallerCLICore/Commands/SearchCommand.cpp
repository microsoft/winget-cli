// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Common.h"
#include "SearchCommand.h"
#include "Localization.h"
#include "Workflows/SearchFlow.h"

namespace AppInstaller::CLI
{
    using namespace AppInstaller::Workflow;

    std::vector<Argument> SearchCommand::GetArguments() const
    {
        return {
            Argument{ ARG_QUERY, LOCME("The query used to search for an app"), ArgumentType::Positional, true },
            Argument{ ARG_ID, LOCME("If specified, filter the results by id"), ArgumentType::Standard },
            Argument{ ARG_NAME, LOCME("If specified, filter the results by name"), ArgumentType::Standard },
            Argument{ ARG_MONIKER, LOCME("If specified, filter the results by app moniker"), ArgumentType::Standard },
            Argument{ ARG_TAG, LOCME("If specified, filter the results by tag"), ArgumentType::Standard },
            Argument{ ARG_COMMAND, LOCME("If specified, filter the results by command"), ArgumentType::Standard },
            Argument{ ARG_SOURCE, LOCME("If specified, find app using the specified source. Default is all source"), ArgumentType::Standard },
            Argument{ ARG_COUNT, LOCME("If specified, find app and show only up to specified number of results."), ArgumentType::Standard },
            Argument{ ARG_EXACT, LOCME("If specified, find app using exact match"), ArgumentType::Flag },
        };
    }

    std::string SearchCommand::ShortDescription() const
    {
        return LOCME("Find and show basic info of apps");
    }

    std::vector<std::string> SearchCommand::GetLongDescription() const
    {
        return {
            LOCME("Find and show basic info of apps"),
        };
    }

    void SearchCommand::ExecuteInternal(Invocation& inv, std::ostream& out, std::istream& in) const
    {
        SearchFlow appInstall(inv, out, in);

        appInstall.Execute();
    }
}
