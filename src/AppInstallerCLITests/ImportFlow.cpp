// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestHooks.h"
#include "WorkflowCommon.h"
#include <Commands/ImportCommand.h>
#include <winget/Settings.h>
#include <Workflows/ImportExportFlow.h>
#include <Workflows/ShellExecuteInstallerHandler.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Utility::literals;

void OverrideForImportSource(TestContext& context, bool useTestCompositeSource = false)
{
    auto testCompositeSource = CreateTestSource({
        TSR::TestInstaller_Exe,
        TSR::TestInstaller_Exe_Dependencies,
        TSR::TestInstaller_Exe_LicenseAgreement,
        TSR::TestInstaller_Exe_NothingInstalled,
        TSR::TestInstaller_Msix,
        TSR::TestInstaller_Msix_WFDependency,
        });

    context.Override({ "OpenPredefinedSource", [=](TestContext& context)
    {
        auto installedSource = useTestCompositeSource ? testCompositeSource : std::make_shared<TestSource>();
        context.Add<Execution::Data::Source>(Source{ installedSource });
    } });

    context.Override({ Workflow::OpenSourcesForImport, [=](TestContext& context)
    {
        context.Add<Execution::Data::Sources>(std::vector<Source>{ Source{ testCompositeSource } });
    } });
}

TEST_CASE("ImportFlow_Successful", "[ImportFlow][workflow]")
{
    TestCommon::TempFile exeInstallResultPath("TestExeInstalled.txt");
    TestCommon::TempFile msixInstallResultPath("TestMsixInstalled.txt");

    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForImportSource(context);
    OverrideForMSIX(context);
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Good.json").GetPath().string());

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Verify all packages were installed
    REQUIRE(std::filesystem::exists(exeInstallResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(msixInstallResultPath.GetPath()));
}

TEST_CASE("ImportFlow_PackageAlreadyInstalled", "[ImportFlow][workflow]")
{
    TestCommon::TempFile exeInstallResultPath("TestExeInstalled.txt");

    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForImportSource(context, true);
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Good-AlreadyInstalled.json").GetPath().string());

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Exe should not have been installed again
    REQUIRE(!std::filesystem::exists(exeInstallResultPath.GetPath()));
    REQUIRE(importOutput.str().find(Resource::LocString(Resource::String::MultiQueryPackageAlreadyInstalled("AppInstallerCliTest.TestExeInstaller"_liv)).get()) != std::string::npos);
}

TEST_CASE("ImportFlow_IgnoreVersions", "[ImportFlow][workflow]")
{
    TestCommon::TempFile exeInstallResultPath("TestExeInstalled.txt");

    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForImportSource(context);
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Good-AlreadyInstalled.json").GetPath().string());
    context.Args.AddArg(Execution::Args::Type::IgnoreVersions);

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Specified version is already installed. It should have been updated since we ignored the version.
    REQUIRE(std::filesystem::exists(exeInstallResultPath.GetPath()));
}

TEST_CASE("ImportFlow_MissingSource", "[ImportFlow][workflow]")
{
    TestCommon::TempFile exeInstallResultPath("TestExeInstalled.txt");

    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Bad-UnknownSource.json").GetPath().string());

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Installer should not be called
    REQUIRE(!std::filesystem::exists(exeInstallResultPath.GetPath()));
    REQUIRE(importOutput.str().find(Resource::LocString(Resource::String::ImportSourceNotInstalled("TestSource"_liv)).get()) != std::string::npos);
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_SOURCE_NAME_DOES_NOT_EXIST);
}

TEST_CASE("ImportFlow_MissingPackage", "[ImportFlow][workflow]")
{
    TestCommon::TempFile exeInstallResultPath("TestExeInstalled.txt");

    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForImportSource(context);
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Bad-UnknownPackage.json").GetPath().string());

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Installer should not be called
    REQUIRE(!std::filesystem::exists(exeInstallResultPath.GetPath()));
    REQUIRE(importOutput.str().find(Resource::LocString(Resource::String::MultiQueryPackageNotFound("MissingPackage"_liv)).get()) != std::string::npos);
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_NOT_ALL_QUERIES_FOUND_SINGLE);
}

TEST_CASE("ImportFlow_IgnoreMissingPackage", "[ImportFlow][workflow]")
{
    TestCommon::TempFile exeInstallResultPath("TestExeInstalled.txt");

    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForImportSource(context);
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Bad-UnknownPackage.json").GetPath().string());
    context.Args.AddArg(Execution::Args::Type::IgnoreUnavailable);

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Verify installer was called for the package that was available.
    REQUIRE(std::filesystem::exists(exeInstallResultPath.GetPath()));
    REQUIRE(importOutput.str().find(Resource::LocString(Resource::String::MultiQueryPackageNotFound("MissingPackage"_liv)).get()) != std::string::npos);
}

TEST_CASE("ImportFlow_MissingVersion", "[ImportFlow][workflow]")
{
    TestCommon::TempFile exeInstallResultPath("TestExeInstalled.txt");

    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForImportSource(context);
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Bad-UnknownPackageVersion.json").GetPath().string());

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Installer should not be called
    REQUIRE(!std::filesystem::exists(exeInstallResultPath.GetPath()));
    REQUIRE(importOutput.str().find(Resource::LocString(Resource::String::MultiQuerySearchFailed("AppInstallerCliTest.TestExeInstaller"_liv)).get()) != std::string::npos);
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_NOT_ALL_QUERIES_FOUND_SINGLE);
}

TEST_CASE("ImportFlow_MalformedJsonFile", "[ImportFlow][workflow]")
{
    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Bad-Malformed.json").GetPath().string());

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Command should have failed
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE);
}

TEST_CASE("ImportFlow_InvalidJsonFile", "[ImportFlow][workflow]")
{
    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Bad-Invalid.json").GetPath().string());

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Command should have failed
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE);
}

TEST_CASE("ImportFlow_MachineScope", "[ImportFlow][workflow]")
{
    TestCommon::TempFile exeInstallResultPath("TestExeInstalled.txt");

    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForImportSource(context);
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Good-MachineScope.json").GetPath().string());

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Verify all packages were installed
    REQUIRE(std::filesystem::exists(exeInstallResultPath.GetPath()));
    std::ifstream installResultFile(exeInstallResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/scope=machine") != std::string::npos);
}

TEST_CASE("ImportFlow_Dependencies", "[ImportFlow][workflow][dependencies]")
{
    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForImportSource(context);
    OverrideForMSIX(context);
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Good-Dependencies.json").GetPath().string());

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Verify dependencies for all packages are informed
    REQUIRE(importOutput.str().find(Resource::LocString(Resource::String::ImportCommandReportDependencies).get()) != std::string::npos);
    REQUIRE(importOutput.str().find("PreviewIIS") != std::string::npos);
    REQUIRE(importOutput.str().find("Preview VC Runtime") != std::string::npos);
    REQUIRE(importOutput.str().find("Hyper-V") != std::string::npos);
}

TEST_CASE("ImportFlow_LicenseAgreement", "[ImportFlow][workflow]")
{
    TestCommon::TempFile exeInstallResultPath("TestExeInstalled.txt");

    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForImportSource(context);
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Good-WithLicenseAgreement.json").GetPath().string());
    context.Args.AddArg(Execution::Args::Type::AcceptPackageAgreements);

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Verify agreements are shown
    REQUIRE(importOutput.str().find("Agreement for EXE") != std::string::npos);
    REQUIRE(importOutput.str().find("This is the agreement for the EXE") != std::string::npos);

    // Verify all packages were installed
    REQUIRE(std::filesystem::exists(exeInstallResultPath.GetPath()));
}

TEST_CASE("ImportFlow_LicenseAgreement_NotAccepted", "[ImportFlow][workflow]")
{
    // Say "No" at the agreements prompt
    std::istringstream importInput{ "n" };

    std::ostringstream importOutput;
    TestContext context{ importOutput, importInput };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForImportSource(context);
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Good-WithLicenseAgreement.json").GetPath().string());

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Verify agreements are shown
    REQUIRE(importOutput.str().find("Agreement for EXE") != std::string::npos);
    REQUIRE(importOutput.str().find("This is the agreement for the EXE") != std::string::npos);

    // Command should have failed
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_PACKAGE_AGREEMENTS_NOT_ACCEPTED);
}
