#pragma once
#include "ExecutionContext.h"
#include "Public/Caller.h"

namespace AppInstaller::CLI
{
	int ExecuteCommand(std::vector<std::string> utf8Args, AppInstallerCaller caller, Execution::Context& context);
}
