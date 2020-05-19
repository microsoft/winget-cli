// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "Commands/HashCommand.h"

using namespace std::string_literals;
using namespace TestCommon;
using namespace AppInstaller::CLI;

TEST_CASE("HashCommandWithTestMsix", "[Sha256Hash]")
{
    std::ostringstream hashOutput;
    Execution::Context context{ hashOutput, std::cin };
    context.Args.AddArg(Execution::Args::Type::HashFile, TestDataFile("TestSignedApp.msix").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::Msix);
    HashCommand hashCommand({});

    hashCommand.Execute(context);

    REQUIRE(hashOutput.str().find("Sha256: 6a2d3683fa19bf00e58e07d1313d20a5f5735ebbd6a999d33381d28740ee07ea") != std::string::npos);
    REQUIRE(hashOutput.str().find("SignatureSha256: 138781c3e6f635240353f3d14d1d57bdcb89413e49be63b375e6a5d7b93b0d07") != std::string::npos);
}