// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include <AppInstallerCLICore.h>
#include <iostream>
#include <string_view>
#include "Caller.h"

// These changes are only for local test purpose
// They will reverted once the core changes are locked down
// COMProgressSink and Install(std::wstring_view appToInstall) is what COM Interface would do to call into winget

using namespace AppInstaller;
using namespace AppInstaller::CLI;

struct COMProgressSink : IProgressSink
{
    COMProgressSink() = default;

    void OnProgress(uint64_t current, uint64_t maximum, ProgressType type) override
    {
        // COM Interface will call the Caller passed in Callback, here

        // Rest of this method is just Local Testing and Demo
        std::cout << "current: ";
        std::cout << current;
        std::cout << "   maximum: ";
        std::cout << maximum;
        std::cout << "   ProgressType : ";
        std::cout << (uint32_t)type;
        std::cout << "\n";

        if (current == maximum)
        {
            // Current Execution Phase is Complete
            std::cout << "Current Execution Phase is now complete\n";
        }
    }

    void OnExecutionStageChange(uint32_t executionStage) override
    {
        // COM Interface will call the Caller passed in Callback

        // Rest of this method is just Local Testing and Demo
        std::cout << "Execution Stage:";
        std::cout << executionStage;
        std::cout << "\n";
    }
};

int COMInstall(std::wstring_view appToInstall, AppInstallerCaller caller) try
{
    COMProgressSink comPS;

    // COM Caller should do what the below method does by invoking registered AppInstaller call back in ProgressCallBack object
    // context.Reporter.CancelInProgressTask(true);
    int Hr = Install(appToInstall, caller, comPS);
    if (Hr == S_OK)
    {
        // Successfully installed or updated the Win32 app
    }
    // Set Hr and return result to COM interface caller on WaitAsync
    return Hr;
}
catch (...)
{
    // Set Hr and return to COM interface caller on WaitAsync
    return 1;
}

// These changes are only for local test purpose
// They will reverted once the core changes are locked down
// int wmain(int argc, wchar_t const** argv)
int wmain()
{
    std::wstring_view appToInstall{ L"Mozilla.Firefox" };
    return COMInstall(appToInstall, AppInstallerCaller::NoCLI);
    //return AppInstaller::CLI::CoreMain(argc, argv);
}
