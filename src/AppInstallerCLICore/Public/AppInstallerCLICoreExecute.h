#pragma once
#include "ExecutionContext.h"
#include "Command.h"

namespace AppInstaller::CLI
{
    int Execute(Execution::Context& context, std::unique_ptr<Command>& command);
}