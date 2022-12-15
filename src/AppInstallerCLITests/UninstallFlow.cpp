// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include <Commands/UninstallCommand.h>
#include <Workflows/PortableFlow.h>
#include <Workflows/ShellExecuteInstallerHandler.h>
#include <Workflows/UninstallFlow.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Workflow;

void OverrideForPortableUninstall(TestContext& context)
{
    context.Override({ GetUninstallInfo, [](TestContext&)
    {
    } });

    context.Override({ PortableUninstallImpl, [](TestContext& context)
    {
        std::filesystem::path temp = std::filesystem::temp_directory_path();
        temp /= "TestPortableUninstalled.txt";
        std::ofstream file(temp, std::ofstream::out);
        file.close();

        context.Add<Execution::Data::OperationReturnCode>(0);
    } });
}

void OverrideForExeUninstall(TestContext& context)
{
    context.Override({ ShellExecuteUninstallImpl, [](TestContext& context)
    {
            // Write out the uninstall command
            std::filesystem::path temp = std::filesystem::temp_directory_path();
            temp /= "TestExeUninstalled.txt";
            std::ofstream file(temp, std::ofstream::out);
            file << context.Get<Execution::Data::UninstallString>();
            file.close();

            context.Add<Execution::Data::OperationReturnCode>(0);
        } });
}

void OverrideForMSIXUninstall(TestContext& context)
{
    context.Override({ MsixUninstall, [](TestContext& context)
    {
            // Write out the package full name
            std::filesystem::path temp = std::filesystem::temp_directory_path();
            temp /= "TestMsixUninstalled.txt";
            std::ofstream file(temp, std::ofstream::out);
            for (const auto& packageFamilyName : context.Get<Execution::Data::PackageFamilyNames>())
            {
                file << packageFamilyName << std::endl;
            }

            file.close();
        } });
}

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
