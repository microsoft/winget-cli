// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include <Commands/DownloadCommand.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;

TEST_CASE("DownloadFlow_DownloadCommandProhibited", "[DownloadFlow][workflow]")
{
    std::ostringstream downloadOutput;
    TestContext context{ downloadOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("DownloadFlowTest_DownloadCommandProhibited.yaml").GetPath().u8string());

    DownloadCommand download({});
    download.Execute(context);
    INFO(downloadOutput.str());

    // Verify AppInfo is printed
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_DOWNLOAD_COMMAND_PROHIBITED);
    REQUIRE(downloadOutput.str().find(Resource::LocString(Resource::String::InstallerDownloadCommandProhibited).get()) != std::string::npos);
}
