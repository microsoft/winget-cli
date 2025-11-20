// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PackageManager.h"
#include "Tests.h"

using namespace std::string_view_literals;

#define ADVANCE_ARG_PARAMETER if (++i >= argc) { return E_INVALIDARG; }

std::string ToLower(std::string_view in)
{
    std::string result(in);
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

int main(int argc, const char** argv) try
{
    std::string testToRun;
    std::string comInit = "mta";
    bool leakCOM = false;
    int iterations = 1;
    std::string packageName = "Microsoft.Edit";
    // bool preventUnload = false; TODO: Add ability to prevent unload and allow testing here

    for (int i = 0; i < argc; ++i)
    {
        if ("-test"sv == argv[i])
        {
            ADVANCE_ARG_PARAMETER
            testToRun = ToLower(argv[i]);
        }
        else if ("-com"sv == argv[i])
        {
            ADVANCE_ARG_PARAMETER
            comInit = ToLower(argv[i]);
        }
        else if ("-leak-com"sv == argv[i])
        {
            leakCOM = true;
        }
        else if ("-itr"sv == argv[i])
        {
            ADVANCE_ARG_PARAMETER
            iterations = atoi(argv[i]);
        }
        else if ("-pkg"sv == argv[i])
        {
            ADVANCE_ARG_PARAMETER
            packageName = argv[i];
        }
    }

    std::cout << "Running inproc testbed with:\n"
        "  COM Init: " << comInit << "\n"
        "  Leak COM: " << (leakCOM ? "true" : "false") << "\n"
        "  Test    : " << testToRun << "\n"
        "  Package : " << packageName << "\n"
        "  Passes  : " << iterations << std::endl;

    HRESULT hr = S_OK;

    if ("sta"sv == comInit)
    {
        hr = RoInitialize(RO_INIT_SINGLETHREADED);
    }
    else if ("mta"sv == comInit)
    {
        hr = RoInitialize(RO_INIT_MULTITHREADED);
    }
    // else no COM init means "let C++/WinRT do it"

    if (FAILED(hr))
    {
        std::cout << "RoInitialize returned " << hr << std::endl;
        return 1;
    }

    for (int i = 0; i < iterations; ++i)
    {
        if (!UsePackageManager(packageName))
        {
            return 2;
        }

        if ("unload_check"sv == testToRun)
        {
            UnloadAndCheckForLeaks();
        }

        std::cout << "Iteration " << (i + 1) << " completed" << std::endl;
    }

    if (!leakCOM)
    {
        RoUninitialize();
    }

    std::cout << "Tests completed" << std::endl;
    return 0;
}
catch (const std::exception& e)
{
    std::cout << "Caught std exception: " << e.what() << std::endl;
    return 3;
}
catch (const winrt::hresult_error& hre)
{
    std::cout << "Caught winrt exception: " << winrt::to_string(hre.message()) << std::endl;
    return 3;
}
catch (...)
{
    std::cout << "Caught unknown exception" << std::endl;
    return 3;
}
