// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Commands/RootCommand.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace AppInstaller::CLI;

int wmain(int argc, wchar_t const** argv) try
{
    init_apartment();
    RegisterTraceLogging();

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
        UnRegisterTraceLogging();
        return 1;
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

    UnRegisterTraceLogging();
}
// End of the line exceptions that are not ever expected
catch (const winrt::hresult_error&)
{

}
catch (const std::exception&)
{

}
