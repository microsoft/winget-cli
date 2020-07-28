// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <Command.h>
#include <Commands/CompleteCommand.h>
#include <CompletionData.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace TestCommon;
using namespace AppInstaller;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Execution;


TEST_CASE("CompletionData", "[complete]")
{
    Args args;
    TestCommand command({
            Argument{ "pos1", 'p', Args::Type::Channel, DefaultDesc, ArgumentType::Positional },
            Argument{ "std1", 's', Args::Type::Command, DefaultDesc, ArgumentType::Standard },
            Argument{ "pos2", 'q', Args::Type::Count, DefaultDesc, ArgumentType::Positional },
            Argument{ "flag1", 'f', Args::Type::Exact, DefaultDesc, ArgumentType::Flag },
        });

    std::vector<std::string> values{ "--pos2" };
    Invocation inv{ std::vector<std::string>(values) };

    REQUIRE_COMMAND_EXCEPTION(command.ParseArguments(inv, args), values[0]);
}
