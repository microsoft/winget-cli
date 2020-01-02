// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#define CATCH_CONFIG_RUNNER
#include <catch.hpp>
#include <winrt/Windows.Foundation.h>
#include <string>
#include <vector>

#include <Public/AppInstallerLogging.h>
#include <Public/AppInstallerTelemetry.h>

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

    // Enable all logging, to force log string building to run.
    // By not creating a log target, it will all be thrown away.
    AppInstaller::Logging::Log().EnableChannel(AppInstaller::Logging::Channel::All);
    AppInstaller::Logging::Log().SetLevel(AppInstaller::Logging::Level::Verbose);
    AppInstaller::Logging::EnableWilFailureTelemetry();

    return Catch::Session().run(static_cast<int>(args.size()), args.data());
}
