// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include <Commands/ShowCommand.h>
#include <Workflows/ShowFlow.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;

TEST_CASE("ShowFlow_SearchAndShowAppInfo", "[ShowFlow][workflow]")
{
    std::ostringstream showOutput;
    TestContext context{ showOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForOpenSource(context, CreateTestSource({ TSR::TestQuery_ReturnOne }));
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestQuery_ReturnOne.Query);

    ShowCommand show({});
    show.Execute(context);
    INFO(showOutput.str());

    // Verify AppInfo is printed
    REQUIRE(showOutput.str().find("AppInstallerCliTest.TestExeInstaller") != std::string::npos);
    REQUIRE(showOutput.str().find("AppInstaller Test Exe Installer") != std::string::npos);
    REQUIRE(showOutput.str().find("1.0.0.0") != std::string::npos);
    REQUIRE(showOutput.str().find("https://ThisIsNotUsed") != std::string::npos);
}

TEST_CASE("ShowFlow_SearchAndShowAppVersion", "[ShowFlow][workflow]")
{
    std::ostringstream showOutput;
    TestContext context{ showOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForOpenSource(context, CreateTestSource({ TSR::TestQuery_ReturnOne }));
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestQuery_ReturnOne.Query);
    context.Args.AddArg(Execution::Args::Type::ListVersions);

    ShowCommand show({});
    show.Execute(context);
    INFO(showOutput.str());

    // Verify App version is printed
    REQUIRE(showOutput.str().find("1.0.0.0") != std::string::npos);
    // No manifest info is printed
    REQUIRE(showOutput.str().find("  Download Url: https://ThisIsNotUsed") == std::string::npos);
}

TEST_CASE("ShowFlow_Dependencies", "[ShowFlow][workflow][dependencies]")
{
    std::ostringstream showOutput;
    TestContext context{ showOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("Manifest-Good-AllDependencyTypes.yaml").GetPath().u8string());

    ShowCommand show({});
    show.Execute(context);
    INFO(showOutput.str());

    // Verify all types of dependencies are printed
    REQUIRE(showOutput.str().find("Dependencies") != std::string::npos);
    REQUIRE(showOutput.str().find("WindowsFeaturesDep") != std::string::npos);
    REQUIRE(showOutput.str().find("WindowsLibrariesDep") != std::string::npos);
    // PackageDep1 has minimum version (1.0), PackageDep2 doesn't (shouldn't show [>=...])
    REQUIRE(showOutput.str().find("Package.Dep1-x64 [>= 1.0]") != std::string::npos);
    REQUIRE(showOutput.str().find("Package.Dep2-x64") != std::string::npos);
    REQUIRE(showOutput.str().find("Package.Dep2-x64 [") == std::string::npos);
    REQUIRE(showOutput.str().find("ExternalDep") != std::string::npos);
}

TEST_CASE("ShowFlow_InstallerType", "[ShowFlow][workflow]")
{
    std::ostringstream showOutput;
    TestContext context{ showOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());

    ShowCommand show({});
    show.Execute(context);
    INFO(showOutput.str());

    // Verify that just the base installed type is shown;
    REQUIRE(showOutput.str().find(Resource::LocString(Resource::String::ShowLabelInstallerType)) != std::string::npos);
    REQUIRE(showOutput.str().find("exe") != std::string::npos);

    // If the base installer is incorrectly shown, an open parenthesis would appear after the effective installer type
    REQUIRE(showOutput.str().find("exe (") == std::string::npos);
}

TEST_CASE("ShowFlow_NestedInstallerType", "[ShowFlow][workflow]")
{
    std::ostringstream showOutput;
    TestContext context{ showOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Zip_Exe.yaml").GetPath().u8string());

    ShowCommand show({});
    show.Execute(context);
    INFO(showOutput.str());

    // Verify that both the effective and base installer types are shown
    REQUIRE(showOutput.str().find(Resource::LocString(Resource::String::ShowLabelInstallerType)) != std::string::npos);
    REQUIRE(showOutput.str().find("exe (zip)") != std::string::npos);
}
