// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerCLICore.h"
#include "Commands/RootCommand.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace AppInstaller::CLI;

int CLICoreMain(int argc, wchar_t const** argv) try
{
    init_apartment();
    TraceLoggingRegistration tlRegistration;

    RootCommand root;
    Invocation invocation{ argc, argv };

    // The root command is our fallback in the event of very bad or very little input
    Command* commandToExecute = &root;
    std::unique_ptr<Command> foundCommand;

    try
    {
        foundCommand = root.FindInvokedCommand(invocation);
        if (foundCommand)
        {
            commandToExecute = foundCommand.get();
        }
        commandToExecute->ParseArguments(invocation);
        commandToExecute->ValidateArguments(invocation);
    }
    // Exceptions specific to parsing the arguments of a command
    catch (const CommandException& ce)
    {
        commandToExecute->OutputHelp(std::wcout, &ce);
        return CLICORE_ERROR_INVALID_CL_ARGUMENTS;
    }

    try
    {
        commandToExecute->Execute(invocation, std::wcout);
    }
    // Exceptions that may occur in the process of executing an arbitrary command
    catch (const winrt::hresult_error&)
    {

    }
    catch (const std::exception&)
    {

    }

    return 0;
}
// End of the line exceptions that are not ever expected.
// Telemetry cannot be reliable beyond this point, so don't let these happen.
catch (const winrt::hresult_error&)
{
    return CLICORE_ERROR_INTERNAL_ERROR;
}
catch (const std::exception&)
{
    return CLICORE_ERROR_INTERNAL_ERROR;
}
