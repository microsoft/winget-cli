// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include "TestHooks.h"
#include <Commands/InstallCommand.h>
#include <Commands/UninstallCommand.h>
#include <Commands/UpgradeCommand.h>
#include <winget/PathVariable.h>
#include <Workflows/ShellExecuteInstallerHandler.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Utility::literals;

TEST_CASE("UpdateFlow_UpdateWithManifest", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe }));
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("UpdateFlowTest_Exe.yaml").GetPath().u8string());

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(updateResultPath.GetPath()));
    std::ifstream updateResultFile(updateResultPath.GetPath());
    REQUIRE(updateResultFile.is_open());
    std::string updateResultStr;
    std::getline(updateResultFile, updateResultStr);
    REQUIRE(updateResultStr.find("/update") != std::string::npos);
    REQUIRE(updateResultStr.find("/silentwithprogress") != std::string::npos);
}

TEST_CASE("UpdateFlow_UpdateWithManifestMSStore", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestMSStoreUpdated.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_MSStore }));
    OverrideForMSStore(context, true);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_MSStore.yaml").GetPath().u8string());

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(updateResultPath.GetPath()));
    std::ifstream updateResultFile(updateResultPath.GetPath());
    REQUIRE(updateResultFile.is_open());
    std::string updateResultStr;
    std::getline(updateResultFile, updateResultStr);
    REQUIRE(updateResultStr.find("9WZDNCRFJ364") != std::string::npos);
}

TEST_CASE("UpdateFlow_UpdateWithManifestAppNotInstalled", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({}));
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallerArgTest_Inno_NoSwitches.yaml").GetPath().u8string());

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is not called.
    REQUIRE(!std::filesystem::exists(updateResultPath.GetPath()));
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::NoInstalledPackageFound).get()) != std::string::npos);
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_NO_APPLICATIONS_FOUND);
}

TEST_CASE("UpdateFlow_UpdateWithManifestVersionAlreadyInstalled", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe }));
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is not called.
    REQUIRE(!std::filesystem::exists(updateResultPath.GetPath()));
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::UpdateNoPackagesFound).get()) != std::string::npos);
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::UpdateNoPackagesFoundReason).get()) != std::string::npos);
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
}

TEST_CASE("UpdateFlow_UpdateExe", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe }));
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe.Query);
    context.Args.AddArg(Execution::Args::Type::Silent);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(updateResultPath.GetPath()));
    std::ifstream updateResultFile(updateResultPath.GetPath());
    REQUIRE(updateResultFile.is_open());
    std::string updateResultStr;
    std::getline(updateResultFile, updateResultStr);
    REQUIRE(updateResultStr.find("/update") != std::string::npos);
    REQUIRE(updateResultStr.find("/silence") != std::string::npos);
    REQUIRE(updateResultStr.find("/ver3.0.0.0") != std::string::npos);
}

TEST_CASE("UpdateFlow_UpdateZip_Exe", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Zip }));
    OverrideForShellExecute(context);
    OverrideForExtractInstallerFromArchive(context);
    OverrideForVerifyAndSetNestedInstaller(context);
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Zip.Query);
    context.Args.AddArg(Execution::Args::Type::Silent);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(updateResultPath.GetPath()));
    std::ifstream updateResultFile(updateResultPath.GetPath());
    REQUIRE(updateResultFile.is_open());
    std::string updateResultStr;
    std::getline(updateResultFile, updateResultStr);
    REQUIRE(updateResultStr.find("/custom") != std::string::npos);
    REQUIRE(updateResultStr.find("/silence") != std::string::npos);
    REQUIRE(updateResultStr.find("/ver2.0.0.0") != std::string::npos);
}

TEST_CASE("UpdateFlow_UpdatePortable", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestPortableInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Portable }));
    OverrideForPortableInstallFlow(context);
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Portable.Query);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());
    REQUIRE(std::filesystem::exists(updateResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_Portable_SymlinkCreationFail", "[UpdateFlow][workflow]")
{
    // Update portable with symlink creation failure verify that it succeeds.
    TestCommon::TempDirectory tempDirectory("TestPortableInstallRoot", false);
    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto PreviousThreadGlobals = context.SetForCurrentThread();
    bool overrideCreateSymlinkStatus = false;
    AppInstaller::Filesystem::TestHook_SetCreateSymlinkResult_Override(&overrideCreateSymlinkStatus);
    OverridePortableInstaller(context);
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Portable }));
    const auto& targetDirectory = tempDirectory.GetPath();
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Portable.Query);
    context.Args.AddArg(Execution::Args::Type::InstallLocation, targetDirectory.u8string());
    context.Args.AddArg(Execution::Args::Type::InstallScope, "user"sv);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());
    const auto& portableTargetPath = targetDirectory / "AppInstallerTestExeInstaller.exe";
    REQUIRE(std::filesystem::exists(portableTargetPath));
    REQUIRE(AppInstaller::Registry::Environment::PathVariable(AppInstaller::Manifest::ScopeEnum::User).Contains(targetDirectory));

    // Perform uninstall
    std::ostringstream uninstallOutput;
    TestContext uninstallContext{ uninstallOutput, std::cin };
    auto previousThreadGlobals = uninstallContext.SetForCurrentThread();
    OverrideForCompositeInstalledSource(uninstallContext, CreateTestSource({ TSR::TestInstaller_Portable }));
    uninstallContext.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Portable.Query);

    UninstallCommand uninstall({});
    uninstall.Execute(uninstallContext);
    INFO(uninstallOutput.str());

    REQUIRE_FALSE(std::filesystem::exists(portableTargetPath));
}

TEST_CASE("UpdateFlow_UpdateExeWithUnsupportedArgs", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");
    TestCommon::TempDirectory tempDirectory("TempDirectory", false);

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe_UnsupportedArguments }));
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe_UnsupportedArguments.Query);
    context.Args.AddArg(Execution::Args::Type::InstallLocation, tempDirectory);

    UpgradeCommand update({});
    context.SetExecutingCommand(&update);
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify unsupported arguments error message is shown 
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UNSUPPORTED_ARGUMENT);
    REQUIRE(!std::filesystem::exists(updateResultPath.GetPath()));
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::UnsupportedArgument).get()) != std::string::npos);
    REQUIRE(updateOutput.str().find("-l,--location") != std::string::npos);
}

TEST_CASE("UpdateFlow_UnknownVersion", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");
    TestCommon::TempDirectory tempDirectory("TempDirectory", false);

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe_UnknownVersion }));
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe_UnknownVersion.Query);
    context.Args.AddArg(Execution::Args::Type::InstallLocation, tempDirectory);

    UpgradeCommand update({});
    context.SetExecutingCommand(&update);
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify help message is shown the user to use --include-unknown
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
    REQUIRE(!std::filesystem::exists(updateResultPath.GetPath()));
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::UpgradeUnknownVersionExplanation).get()) != std::string::npos);
}

TEST_CASE("UpdateFlow_UnknownVersion_IncludeUnknownArg", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");
    TestCommon::TempDirectory tempDirectory("TempDirectory", false);

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe_UnknownVersion }));
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe_UnknownVersion.Query);
    context.Args.AddArg(Execution::Args::Type::InstallLocation, tempDirectory);
    context.Args.AddArg(Execution::Args::Type::IncludeUnknown);

    UpgradeCommand update({});
    context.SetExecutingCommand(&update);
    update.Execute(context);
    INFO(updateOutput.str());
    REQUIRE(std::filesystem::exists(updateResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_NoArgs_UnknownVersion", "[UpdateFlow][workflow]")
{
    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({
        TSR::TestInstaller_Exe,
        TSR::TestInstaller_Exe_UnknownVersion,
        TSR::TestInstaller_Msix,
        TSR::TestInstaller_MSStore,
        TSR::TestInstaller_Portable,
        TSR::TestInstaller_Zip,
        }));

    UpgradeCommand update({});
    context.SetExecutingCommand(&update);
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify --include-unknown help text is displayed if update is executed with no args and an unknown version package is available for upgrade.
    REQUIRE(updateOutput.str().find(Resource::String::UpgradeUnknownVersionCount(1)) != std::string::npos);
}

TEST_CASE("UpdateFlow_IncludeUnknown", "[UpdateFlow][workflow]")
{
    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({
        TSR::TestInstaller_Exe,
        TSR::TestInstaller_Exe_UnknownVersion,
        TSR::TestInstaller_Msix,
        TSR::TestInstaller_MSStore,
        TSR::TestInstaller_Portable,
        TSR::TestInstaller_Zip,
        }));
    context.Args.AddArg(Execution::Args::Type::IncludeUnknown);

    UpgradeCommand update({});
    context.SetExecutingCommand(&update);
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify unknown version package is displayed available for upgrade.
    REQUIRE(updateOutput.str().find(Resource::String::UpgradeUnknownVersionCount(1)) == std::string::npos);
    REQUIRE(updateOutput.str().find("unknown") != std::string::npos);
}

TEST_CASE("UpdateFlow_UpdatePortableWithManifest", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestPortableInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Portable }));
    OverrideForPortableInstallFlow(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("UpdateFlowTest_Portable.yaml").GetPath().u8string());

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());
    REQUIRE(std::filesystem::exists(updateResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_UpdateMsix", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestMsixInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Msix }));
    OverrideForMSIX(context);
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Msix.Query);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is called.
    REQUIRE(std::filesystem::exists(updateResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_UpdateMSStore", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestMSStoreUpdated.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_MSStore }));
    OverrideForMSStore(context, true);
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_MSStore.Query);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is called.
    REQUIRE(std::filesystem::exists(updateResultPath.GetPath()));
    std::ifstream updateResultFile(updateResultPath.GetPath());
    REQUIRE(updateResultFile.is_open());
    std::string updateResultStr;
    std::getline(updateResultFile, updateResultStr);
    REQUIRE(updateResultStr.find("9WZDNCRFJ364") != std::string::npos);
}

TEST_CASE("UpdateFlow_UpdateExeLatestAlreadyInstalled", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe_LatestInstalled }));
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe_LatestInstalled.Query);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is not called.
    REQUIRE(!std::filesystem::exists(updateResultPath.GetPath()));
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::UpdateNoPackagesFound).get()) != std::string::npos);
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::UpdateNoPackagesFoundReason).get()) != std::string::npos);
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
}

TEST_CASE("UpdateFlow_UpdateExeInstallerTypeNotApplicable", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe_IncompatibleInstallerType }));
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe_IncompatibleInstallerType.Query);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is not called.
    REQUIRE(!std::filesystem::exists(updateResultPath.GetPath()));
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::UpgradeDifferentInstallTechnologyInNewerVersions).get()) != std::string::npos);
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
}

TEST_CASE("UpdateFlow_UpdateExeInstallerTypeNotApplicableSpecificVersion", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe_IncompatibleInstallerType }));
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe_IncompatibleInstallerType.Query);
    context.Args.AddArg(Execution::Args::Type::Version, "2.0.0.0"sv);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is not called.
    REQUIRE(!std::filesystem::exists(updateResultPath.GetPath()));
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::UpgradeDifferentInstallTechnology).get()) != std::string::npos);
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
}

TEST_CASE("UpdateFlow_UpdateExeWithDifferentInstalledType", "[UpdateFlow][workflow]")
{
    // Tests installer applicability when installed type is different but listed in the manifest
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe_DifferentInstallerType }));
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe_DifferentInstallerType.Query);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is called.
    REQUIRE(context.GetTerminationHR() == S_OK);
    REQUIRE(std::filesystem::exists(updateResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_UpdateExeSpecificVersionNotFound", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe }));
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe.Query);
    context.Args.AddArg(Execution::Args::Type::Version, "1.2.3.4"sv);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is not called.
    REQUIRE(!std::filesystem::exists(updateResultPath.GetPath()));
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::GetManifestResultVersionNotFound("1.2.3.4"_liv)).get()) != std::string::npos);
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_NO_MANIFEST_FOUND);
}

TEST_CASE("UpdateFlow_UpdateExeSpecificVersionNotApplicable", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe_IncompatibleInstallerType }));
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe_IncompatibleInstallerType.Query);
    // This must be 2.0.0.0 since the version would not be an upgrade otherwise
    context.Args.AddArg(Execution::Args::Type::Version, "2.0.0.0"sv);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is not called.
    REQUIRE(!std::filesystem::exists(updateResultPath.GetPath()));
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::UpgradeDifferentInstallTechnology).get()) != std::string::npos);
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
}

TEST_CASE("UpdateFlow_UpdateAllApplicable", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateExeResultPath("TestExeInstalled.txt");
    TestCommon::TempFile updateMsixResultPath("TestMsixInstalled.txt");
    TestCommon::TempFile updateMSStoreResultPath("TestMSStoreUpdated.txt");
    TestCommon::TempFile updatePortableResultPath("TestPortableInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({
        TSR::TestInstaller_Exe,
        TSR::TestInstaller_Exe_UnknownVersion,
        TSR::TestInstaller_Msix,
        TSR::TestInstaller_MSStore,
        TSR::TestInstaller_Portable,
        TSR::TestInstaller_Zip,
        }));
    OverrideForShellExecute(context);
    OverrideForMSIX(context);
    OverrideForMSStore(context, true);
    OverrideForPortableInstall(context);
    context.Args.AddArg(Execution::Args::Type::All);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify that --include-unknown help message is displayed.
    REQUIRE(updateOutput.str().find(Resource::String::UpgradeUnknownVersionCount(1)) != std::string::npos);
    REQUIRE(updateOutput.str().find("AppInstallerCliTest.TestExeUnknownVersion") == std::string::npos);

    // Verify installers are called.
    REQUIRE(std::filesystem::exists(updateExeResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(updateMsixResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(updateMSStoreResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(updatePortableResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_UpdateAll_IncludeUnknown", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateExeResultPath("TestExeInstalled.txt");
    TestCommon::TempFile updateMsixResultPath("TestMsixInstalled.txt");
    TestCommon::TempFile updateMSStoreResultPath("TestMSStoreUpdated.txt");
    TestCommon::TempFile updatePortableResultPath("TestPortableInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({
        TSR::TestInstaller_Exe,
        TSR::TestInstaller_Exe_UnknownVersion,
        TSR::TestInstaller_Msix,
        TSR::TestInstaller_MSStore,
        TSR::TestInstaller_Portable,
        TSR::TestInstaller_Zip,
        }));
    OverrideForShellExecute(context);
    OverrideForMSIX(context);
    OverrideForMSStore(context, true);
    OverrideForPortableInstall(context);
    context.Args.AddArg(Execution::Args::Type::All);
    context.Args.AddArg(Execution::Args::Type::IncludeUnknown);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify that --include-unknown help message is NOT displayed and unknown version package is upgraded.
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::UpgradeUnknownVersionCount).get()) == std::string::npos);
    REQUIRE(updateOutput.str().find("AppInstallerCliTest.TestExeUnknownVersion") != std::string::npos);

    // Verify installers are called.
    REQUIRE(std::filesystem::exists(updateExeResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(updateMsixResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(updateMSStoreResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(updatePortableResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_UpgradeWithDuplicateUpgradeItemsFound", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateExeResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe_UpgradeAllWithDuplicateUpgradeItems }));
    // Installer should only be run once since the 2 upgrade items are same.
    OverrideForShellExecute(context, 1);
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe_UpgradeAllWithDuplicateUpgradeItems.Query);
    context.Args.AddArg(Execution::Args::Type::All);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify installers are called.
    REQUIRE(std::filesystem::exists(updateExeResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_Dependencies", "[UpdateFlow][workflow][dependencies]")
{
    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe_Dependencies }));
    OverrideForShellExecute(context);
    OverrideEnableWindowsFeaturesDependencies(context);

    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe_Dependencies.Query);;

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    std::string updateResultStr = updateOutput.str();

    // Verify dependencies are informed
    REQUIRE(updateResultStr.find(Resource::LocString(Resource::String::PackageRequiresDependencies).get()) != std::string::npos);
    REQUIRE(updateResultStr.find("PreviewIIS") != std::string::npos);
    REQUIRE(updateResultStr.find("Preview VC Runtime") != std::string::npos);
}

TEST_CASE("UpdateFlow_LicenseAgreement", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe_LicenseAgreement }));
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe_LicenseAgreement.Query);
    context.Args.AddArg(Execution::Args::Type::AcceptPackageAgreements);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify agreements are shown
    REQUIRE(updateOutput.str().find("Agreement for EXE") != std::string::npos);
    REQUIRE(updateOutput.str().find("This is the agreement for the EXE") != std::string::npos);

    // Verify Installer is called.
    REQUIRE(std::filesystem::exists(updateResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_LicenseAgreement_NotAccepted", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    // Say "No" at the agreements prompt
    std::istringstream updateInput{ "n" };

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, updateInput };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe_LicenseAgreement }));
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe_LicenseAgreement.Query);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify agreements are shown
    REQUIRE(updateOutput.str().find("Agreement for EXE") != std::string::npos);
    REQUIRE(updateOutput.str().find("This is the agreement for the EXE") != std::string::npos);

    // Verify Installer is not called.
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_PACKAGE_AGREEMENTS_NOT_ACCEPTED);
    REQUIRE_FALSE(std::filesystem::exists(updateResultPath.GetPath()));
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::PackageAgreementsNotAgreedTo).get()) != std::string::npos);
}

TEST_CASE("UpdateFlow_All_LicenseAgreement", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateExeResultPath("TestExeInstalled.txt");
    TestCommon::TempFile updateMsixResultPath("TestMsixInstalled.txt");
    TestCommon::TempFile updateMSStoreResultPath("TestMSStoreUpdated.txt");
    TestCommon::TempFile updatePortableResultPath("TestPortableInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({
        TSR::TestInstaller_Exe_UpgradeUsesAgreements,
        TSR::TestInstaller_Exe_UnknownVersion,
        TSR::TestInstaller_Msix_UpgradeUsesAgreements,
        TSR::TestInstaller_MSStore,
        TSR::TestInstaller_Portable,
        TSR::TestInstaller_Zip,
        }));
    OverrideForShellExecute(context);
    OverrideForMSIX(context);
    OverrideForMSStore(context, true);
    OverrideForPortableInstall(context);
    context.Args.AddArg(Execution::Args::Type::All);
    context.Args.AddArg(Execution::Args::Type::AcceptPackageAgreements);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify agreements are shown
    REQUIRE(updateOutput.str().find("Agreement for EXE") != std::string::npos);
    REQUIRE(updateOutput.str().find("This is the agreement for the EXE") != std::string::npos);
    REQUIRE(updateOutput.str().find("Agreement for MSIX") != std::string::npos);
    REQUIRE(updateOutput.str().find("This is the agreement for the MSIX") != std::string::npos);

    // Verify installers are called.
    REQUIRE(std::filesystem::exists(updateExeResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(updateMsixResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(updateMSStoreResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(updatePortableResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_All_LicenseAgreement_NotAccepted", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateExeResultPath("TestExeInstalled.txt");
    TestCommon::TempFile updateMsixResultPath("TestMsixInstalled.txt");
    TestCommon::TempFile updateMSStoreResultPath("TestMSStoreUpdated.txt");

    // Say "No" at the agreements prompt
    std::istringstream updateInput{ "n" };

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, updateInput };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({
        TSR::TestInstaller_Exe_UpgradeUsesAgreements,
        TSR::TestInstaller_Exe_UnknownVersion,
        TSR::TestInstaller_Msix_UpgradeUsesAgreements,
        TSR::TestInstaller_MSStore,
        TSR::TestInstaller_Portable,
        TSR::TestInstaller_Zip,
        }));
    context.Args.AddArg(Execution::Args::Type::All);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify agreements are shown
    REQUIRE(updateOutput.str().find("Agreement for EXE") != std::string::npos);
    REQUIRE(updateOutput.str().find("This is the agreement for the EXE") != std::string::npos);
    REQUIRE(updateOutput.str().find("Agreement for MSIX") != std::string::npos);
    REQUIRE(updateOutput.str().find("This is the agreement for the MSIX") != std::string::npos);

    // Verify installers are not called.
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_PACKAGE_AGREEMENTS_NOT_ACCEPTED);
    REQUIRE_FALSE(std::filesystem::exists(updateExeResultPath.GetPath()));
    REQUIRE_FALSE(std::filesystem::exists(updateMsixResultPath.GetPath()));
    REQUIRE_FALSE(std::filesystem::exists(updateMSStoreResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_RequireExplicit", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateExeResultPath("TestExeInstalled.txt");
    TestCommon::TempFile updateMsixResultPath("TestMsixInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();

    // Msix package has an update that requires explicit upgrade.
    // Exe, Portable, MSStore, Zip are also listed with an available upgrade.
    OverrideForCompositeInstalledSource(context, CreateTestSource({
        TSR::TestInstaller_Exe,
        TSR::TestInstaller_Exe_UnknownVersion,
        TSR::TestInstaller_Msix_UpgradeRequiresExplicit,
        TSR::TestInstaller_MSStore,
        TSR::TestInstaller_Portable,
        TSR::TestInstaller_Zip,
        }));

    SECTION("List available upgrades")
    {
        UpgradeCommand update({});
        update.Execute(context);
        INFO(updateOutput.str());

        // The package that requires explicit upgrade is listed below the header for pinned packages
        REQUIRE(updateOutput.str().find("AppInstallerCliTest.TestExeInstaller") != std::string::npos);

        auto pinnedPackagesHeaderPosition = updateOutput.str().find(Resource::LocString(Resource::String::UpgradeAvailableForPinned));
        auto pinnedPackageLinePosition = updateOutput.str().find("AppInstallerCliTest.TestMsixInstaller");
        REQUIRE(pinnedPackagesHeaderPosition != std::string::npos);
        REQUIRE(pinnedPackageLinePosition != std::string::npos);
        REQUIRE(pinnedPackagesHeaderPosition < pinnedPackageLinePosition);
        REQUIRE(updateOutput.str().find(Resource::String::UpgradeRequireExplicitCount(1)) == std::string::npos);
    }

    SECTION("Upgrade all except pinned")
    {
        context.Args.AddArg(Args::Type::All);
        OverrideForMSStore(context, true);
        OverrideForPortableInstall(context);
        OverrideForShellExecute(context);
        OverrideForExtractInstallerFromArchive(context);
        OverrideForVerifyAndSetNestedInstaller(context);

        UpgradeCommand update({});
        update.Execute(context);
        INFO(updateOutput.str());

        auto s = updateOutput.str();

        // Verify message is printed for skipped package
        REQUIRE(updateOutput.str().find(Resource::String::UpgradeRequireExplicitCount(1)) != std::string::npos);

        // Verify package is not installed, but all others are
        REQUIRE(std::filesystem::exists(updateExeResultPath.GetPath()));
        REQUIRE(!std::filesystem::exists(updateMsixResultPath.GetPath()));
    }

    SECTION("Upgrade explicitly")
    {
        context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Msix.Query);
        OverrideForMSIX(context);

        UpgradeCommand update({});
        update.Execute(context);
        INFO(updateOutput.str());

        REQUIRE(std::filesystem::exists(updateMsixResultPath.GetPath()));
    }

    // Command should always succeed
    REQUIRE(context.GetTerminationHR() == S_OK);
}

TEST_CASE("InstallFlow_FoundInstalledAndUpgradeAvailable", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe }));
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe.Query);
    context.Args.AddArg(Execution::Args::Type::Silent);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/update") != std::string::npos);
    REQUIRE(installResultStr.find("/ver3.0.0.0") != std::string::npos);
}

TEST_CASE("InstallFlow_FoundInstalledAndUpgradeAvailable_WithNoUpgrade", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe }));
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe.Query);
    context.Args.AddArg(Execution::Args::Type::NoUpgrade);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is not called.
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::PackageAlreadyInstalled).get()) != std::string::npos);
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_PACKAGE_ALREADY_INSTALLED);
}

TEST_CASE("InstallFlow_FoundInstalledAndUpgradeNotAvailable", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe_LatestInstalled }));
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe_LatestInstalled.Query);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is not called.
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::UpdateNoPackagesFound).get()) != std::string::npos);
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::UpdateNoPackagesFoundReason).get()) != std::string::npos);
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
}

TEST_CASE("UpdateFlow_UpdateAll_ForwardArgs", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateExeResultPath("TestExeInstalled.txt");
    TestCommon::TempFile updateMsixResultPath("TestMsixInstalled.txt");
    TestCommon::TempFile updateMSStoreResultPath("TestMSStoreUpdated.txt");
    TestCommon::TempFile updatePortableResultPath("TestPortableInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({
        TSR::TestInstaller_Exe,
        TSR::TestInstaller_Msix,
        TSR::TestInstaller_MSStore,
        TSR::TestInstaller_Portable,
        }));
    OverrideForShellExecute(context);
    OverrideForMSIX(context);
    OverrideForMSStore(context, true);
    OverrideForPortableInstall(context);
    context.Args.AddArg(Execution::Args::Type::All);
    context.Args.AddArg(Execution::Args::Type::Silent);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify installers are called with the silent flags
    REQUIRE(std::filesystem::exists(updateExeResultPath.GetPath()));
    std::ifstream updateExeResultFile(updateExeResultPath.GetPath());
    std::string updateExeResultStr;
    std::getline(updateExeResultFile, updateExeResultStr);
    REQUIRE(updateExeResultStr.find("/silence") != std::string::npos);

    REQUIRE(std::filesystem::exists(updateMsixResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(updateMSStoreResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(updatePortableResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_UpdateMultiple", "[UpdateFlow][workflow][MultiQuery]")
{
    TestCommon::TempFile exeUpdateResultPath("TestExeInstalled.txt");
    TestCommon::TempFile msixUpdateResultPath("TestMsixInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe, TSR::TestInstaller_Msix }));
    OverrideForShellExecute(context);
    OverrideForMSIX(context);
    context.Args.AddArg(Execution::Args::Type::MultiQuery, TSR::TestInstaller_Exe.Query);
    context.Args.AddArg(Execution::Args::Type::MultiQuery, TSR::TestInstaller_Msix.Query);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installers are called called.
    REQUIRE(std::filesystem::exists(exeUpdateResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(exeUpdateResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_UpdateMultiple_NotAllFound", "[UpdateFlow][workflow][MultiQuery]")
{
    TestCommon::TempFile exeUpdateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe }));
    context.Args.AddArg(Execution::Args::Type::MultiQuery, TSR::TestInstaller_Exe.Query);
    context.Args.AddArg(Execution::Args::Type::MultiQuery, TSR::TestInstaller_Msix.Query);

    SECTION("Ignore unavailable")
    {
        OverrideForShellExecute(context);
        context.Args.AddArg(Execution::Args::Type::IgnoreUnavailable);

        UpgradeCommand update({});
        update.Execute(context);
        INFO(updateOutput.str());

        REQUIRE(!context.IsTerminated());
        REQUIRE(std::filesystem::exists(exeUpdateResultPath.GetPath()));
    }
    SECTION("Don't ignore unavailable")
    {
        UpgradeCommand update({});
        update.Execute(context);
        INFO(updateOutput.str());

        REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_NOT_ALL_QUERIES_FOUND_SINGLE);
    }
}

TEST_CASE("UpdateFlow_UpdateWithReboot", "[UpdateFlow][workflow][reboot]")
{
    TestCommon::TestUserSettings testSettings;

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    OverrideForCompositeInstalledSource(context, CreateTestSource({ TSR::TestInstaller_Exe_ExpectedReturnCodes }));

    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe_ExpectedReturnCodes.Query);
    context.Args.AddArg(Execution::Args::Type::AllowReboot);

    context.Override({ AppInstaller::CLI::Workflow::ShellExecuteInstallImpl, [&](TestContext& context)
    {
        // APPINSTALLER_CLI_ERROR_INSTALL_REBOOT_REQUIRED_TO_FINISH (not treated as an installer error)
        context.Add<Data::OperationReturnCode>(9);
    } });

    SECTION("Reboot success")
    {
        TestHook::SetInitiateRebootResult_Override initiateRebootResultOverride(true);

        UpgradeCommand update({});
        update.Execute(context);
        INFO(updateOutput.str());

        REQUIRE_FALSE(context.IsTerminated());
        REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::InitiatingReboot).get()) != std::string::npos);
        REQUIRE_FALSE(updateOutput.str().find(Resource::LocString(Resource::String::FailedToInitiateReboot).get()) != std::string::npos);
    }
    SECTION("Reboot failed")
    {
        TestHook::SetInitiateRebootResult_Override initiateRebootResultOverride(false);

        UpgradeCommand update({});
        update.Execute(context);
        INFO(updateOutput.str());

        REQUIRE_FALSE(context.IsTerminated());
        REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::InitiatingReboot).get()) != std::string::npos);
        REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::FailedToInitiateReboot).get()) != std::string::npos);
    }
}
