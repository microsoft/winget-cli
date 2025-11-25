// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PackageManager.h"
#include "Tests.h"

using namespace std::string_view_literals;

int main(int argc, const char** argv) try
{
    const TestParameters testParameters(argc, argv);
    testParameters.OutputDetails();
    if (!testParameters.InitializeTestState())
    {
        return 2;
    }

    auto test = testParameters.CreateTest();

    for (int i = 0; i < testParameters.Iterations; ++i)
    {
        if (!UsePackageManager(testParameters.PackageName))
        {
            return 3;
        }

        if (testParameters.ClearFactories)
        {
            winrt::clear_factory_cache();
        }

        if (test && !test->RunIteration())
        {
            return 4;
        }

        std::cout << "Iteration " << (i + 1) << " completed" << std::endl;
    }

    if (test && !test->RunFinal())
    {
        return 5;
    }

    testParameters.UninitializeTestState();

    std::cout << "Tests completed" << std::endl;
    return 0;
}
catch (const std::exception& e)
{
    std::cout << "Caught std exception: " << e.what() << std::endl;
    return 1;
}
catch (const winrt::hresult_error& hre)
{
    std::cout << "Caught winrt exception: " << winrt::to_string(hre.message()) << std::endl;
    return 1;
}
catch (...)
{
    std::cout << "Caught unknown exception" << std::endl;
    return 1;
}
