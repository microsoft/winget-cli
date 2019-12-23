// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#define CATCH_CONFIG_RUNNER
#include <catch.hpp>
#include <winrt/Windows.Foundation.h>
#include <string>
#include <vector>

#include "TestCommon.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace std::string_literals;

int main(int argc, char** argv)
{
    init_apartment();

    std::vector<char*> args;
    for (int i = 0; i < argc; ++i)
    {
        if ("-ktf"s == argv[i])
        {
            TestCommon::TempFile::SetDestructorBehavior(true);
        }
        else
        {
            args.push_back(argv[i]);
        }
    }

    return Catch::Session().run(static_cast<int>(args.size()), args.data());
}
