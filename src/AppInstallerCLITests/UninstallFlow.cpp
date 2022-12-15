// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"

TEST_CASE("UninstallFlow_UninstallPortable", "[UninstallFlow][workflow]")
{
    TestCommon::TempFile uninstallResultPath("TestPortableUninstalled.txt");

    std::ostringstream uninstallOutput;
    TestContext context{ uninstallOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    OverrideForPortableUninstall(context);
    context.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestPortableInstaller"sv);

    UninstallCommand uninstall({});
    uninstall.Execute(context);
    INFO(uninstallOutput.str());
    REQUIRE(std::filesystem::exists(uninstallResultPath.GetPath()));
}

TEST_CASE("UninstallFlow_UninstallExe", "[UninstallFlow][workflow]")
{
    TestCommon::TempFile uninstallResultPath("TestExeUninstalled.txt");

    std::ostringstream uninstallOutput;
    TestContext context{ uninstallOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    OverrideForExeUninstall(context);
    context.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestExeInstaller"sv);
    context.Args.AddArg(Execution::Args::Type::Silent);

    UninstallCommand uninstall({});
    uninstall.Execute(context);
    INFO(uninstallOutput.str());

    // Verify Uninstaller is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(uninstallResultPath.GetPath()));
    std::ifstream uninstallResultFile(uninstallResultPath.GetPath());
    REQUIRE(uninstallResultFile.is_open());
    std::string uninstallResultStr;
    std::getline(uninstallResultFile, uninstallResultStr);
    REQUIRE(uninstallResultStr.find("uninstall.exe") != std::string::npos);
    REQUIRE(uninstallResultStr.find("/silence") != std::string::npos);
}

TEST_CASE("UninstallFlow_UninstallMsix", "[UninstallFlow][workflow]")
{
    TestCommon::TempFile uninstallResultPath("TestMsixUninstalled.txt");

    std::ostringstream uninstallOutput;
    TestContext context{ uninstallOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    OverrideForMSIXUninstall(context);
    context.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestMsixInstaller"sv);

    UninstallCommand uninstall({});
    uninstall.Execute(context);
    INFO(uninstallOutput.str());

    // Verify Uninstaller is called with the package full name.
    REQUIRE(std::filesystem::exists(uninstallResultPath.GetPath()));
    std::ifstream uninstallResultFile(uninstallResultPath.GetPath());
    REQUIRE(uninstallResultFile.is_open());
    std::string uninstallResultStr;
    std::getline(uninstallResultFile, uninstallResultStr);
    REQUIRE(uninstallResultStr.find("20477fca-282d-49fb-b03e-371dca074f0f_8wekyb3d8bbwe") != std::string::npos);
}

TEST_CASE("UninstallFlow_UninstallMSStore", "[UninstallFlow][workflow]")
{
    TestCommon::TempFile uninstallResultPath("TestMsixUninstalled.txt");

    std::ostringstream uninstallOutput;
    TestContext context{ uninstallOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    OverrideForMSIXUninstall(context);
    context.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestMSStoreInstaller"sv);

    UninstallCommand uninstall({});
    uninstall.Execute(context);
    INFO(uninstallOutput.str());

    // Verify Uninstaller is called with the package full name
    REQUIRE(std::filesystem::exists(uninstallResultPath.GetPath()));
    std::ifstream uninstallResultFile(uninstallResultPath.GetPath());
    REQUIRE(uninstallResultFile.is_open());
    std::string uninstallResultStr;
    std::getline(uninstallResultFile, uninstallResultStr);
    REQUIRE(uninstallResultStr.find("microsoft.skypeapp_kzf8qxf38zg5c") != std::string::npos);
}

TEST_CASE("UninstallFlow_UninstallExeNotFound", "[UninstallFlow][workflow]")
{
    TestCommon::TempFile uninstallResultPath("TestExeUninstalled.txt");

    std::ostringstream uninstallOutput;
    TestContext context{ uninstallOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    context.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.MissingApp"sv);
    context.Args.AddArg(Execution::Args::Type::Silent);

    UninstallCommand uninstall({});
    uninstall.Execute(context);
    INFO(uninstallOutput.str());

    // Verify Uninstaller is not called.
    REQUIRE(!std::filesystem::exists(uninstallResultPath.GetPath()));
    REQUIRE(uninstallOutput.str().find(Resource::LocString(Resource::String::NoInstalledPackageFound).get()) != std::string::npos);
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_NO_APPLICATIONS_FOUND);
}
