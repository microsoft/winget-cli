// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#define CATCH_CONFIG_RUNNER
#include <catch.hpp>
#include <winrt/Windows.Foundation.h>

using namespace winrt;
using namespace Windows::Foundation;

int main(int argc, char** argv)
{
    init_apartment();
    return Catch::Session().run(argc, argv);
}
