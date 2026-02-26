// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include "TestSettings.h"
#include <AppInstallerDownloader.h>
#include <winget/Settings.h>
#include <Workflows/DownloadFlow.h>
#include <Commands/InstallCommand.h>
#include <Commands/SettingsCommand.h>
#include <Commands/ValidateCommand.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Utility;

void VerifyMotw(const std::filesystem::path& testFile, DWORD zone)
{
    std::filesystem::path motwFile(testFile);
    motwFile += ":Zone.Identifier:$data";
    std::ifstream motwStream(motwFile);
    std::stringstream motwContent;
    motwContent << motwStream.rdbuf();
    std::string motwContentStr = motwContent.str();
    motwStream.close();
    REQUIRE(motwContentStr.find("ZoneId=" + std::to_string(zone)) != std::string::npos);
}

TEST_CASE("VerifyInstallerTrustLevelAndUpdateInstallerFileMotw", "[DownloadInstaller][workflow]")
{
    TestCommon::TempFile testInstallerPath("TestInstaller.txt");

    std::ofstream ofile(testInstallerPath, std::ofstream::out);
    ofile << "test";
    ofile.close();

    ApplyMotwIfApplicable(testInstallerPath, URLZONE_INTERNET);
    VerifyMotw(testInstallerPath, 3);

    std::ostringstream updateMotwOutput;
    TestContext context{ updateMotwOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Add<Data::DownloadHashInfo>({ {}, {} });
    context.Add<Data::InstallerPath>(testInstallerPath);
    auto packageVersion = std::make_shared<TestPackageVersion>(Manifest{});
    auto testSource = std::make_shared<TestSource>();
    testSource->Details.TrustLevel = SourceTrustLevel::Trusted;
    packageVersion->Source = testSource;
    context.Add<Data::PackageVersion>(packageVersion);
    ManifestInstaller installer;
    installer.Url = "http://NotTrusted.com";
    context.Add<Data::Installer>(std::move(installer));

    context << VerifyInstallerHash << UpdateInstallerFileMotwIfApplicable;
    REQUIRE(WI_IsFlagSet(context.GetFlags(), ContextFlag::InstallerTrusted));
    VerifyMotw(testInstallerPath, 2);

    testSource->Details.TrustLevel = SourceTrustLevel::None;
    context.ClearFlags(ContextFlag::InstallerTrusted);
    context << VerifyInstallerHash << UpdateInstallerFileMotwIfApplicable;
    REQUIRE_FALSE(WI_IsFlagSet(context.GetFlags(), ContextFlag::InstallerTrusted));
    VerifyMotw(testInstallerPath, 3);

    INFO(updateMotwOutput.str());
}

TEST_CASE("ValidateCommand_Dependencies", "[workflow][dependencies]")
{
    std::ostringstream validateOutput;
    TestContext context{ validateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Args::Type::ValidateManifest, TestDataFile("Manifest-Good-AllDependencyTypes.yaml").GetPath().u8string());

    ValidateCommand validate({});
    validate.Execute(context);
    INFO(validateOutput.str());

    // Verify all types of dependencies are printed
    REQUIRE(validateOutput.str().find(Resource::LocString(Resource::String::ValidateCommandReportDependencies).get()) != std::string::npos);
    REQUIRE(validateOutput.str().find("WindowsFeaturesDep") != std::string::npos);
    REQUIRE(validateOutput.str().find("WindowsLibrariesDep") != std::string::npos);
    // PackageDep1 has minimum version (1.0), PackageDep2 doesn't (shouldn't show [>=...])
    REQUIRE(validateOutput.str().find("Package.Dep1-x64 [>= 1.0]") != std::string::npos);
    REQUIRE(validateOutput.str().find("Package.Dep2-x64") != std::string::npos);
    REQUIRE(validateOutput.str().find("Package.Dep2-x64 [") == std::string::npos);
    REQUIRE(validateOutput.str().find("ExternalDep") != std::string::npos);
}

TEST_CASE("AdminSetting_LocalManifestFiles", "[LocalManifests][workflow]")
{
    RemoveSetting(Stream::AdminSettings);

    {
        // If there's no admin setting, using local manifest should fail.
        Execution::Args args;
        args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());
        InstallCommand installCommand({});
        REQUIRE_THROWS(installCommand.ValidateArguments(args));
    }

    {
        // Using settings command to enable local manifests
        std::ostringstream settingsOutput;
        TestContext context{ settingsOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        context.Args.AddArg(Execution::Args::Type::AdminSettingEnable, "LocalManifestFiles"sv);
        context.Override({ EnsureRunningAsAdmin, [](TestContext&) {} });
        SettingsCommand settings({});
        settings.Execute(context);
        INFO(settingsOutput.str());
    }

    {
        // Now using local manifests should succeed
        Execution::Args args2;
        args2.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());
        InstallCommand installCommand2({});
        REQUIRE_NOTHROW(installCommand2.ValidateArguments(args2));
    }

    {
        // Using settings command to disable local manifests
        std::ostringstream settingsOutput2;
        TestContext context2{ settingsOutput2, std::cin };
        auto previousThreadGlobals = context2.SetForCurrentThread();
        context2.Args.AddArg(Execution::Args::Type::AdminSettingDisable, "LocalManifestFiles"sv);
        context2.Override({ EnsureRunningAsAdmin, [](TestContext&) {} });
        SettingsCommand settings2({});
        settings2.Execute(context2);
        INFO(settingsOutput2.str());
    }

    {
        // Now using local manifests should fail
        Execution::Args args3;
        args3.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());
        InstallCommand installCommand3({});
        REQUIRE_THROWS(installCommand3.ValidateArguments(args3));
    }
}

TEST_CASE("Export_Settings", "[Settings][workflow]")
{
    RemoveSetting(Stream::AdminSettings);

    {
        // No admin settings, local manifest should be false.
        std::ostringstream exportOutput;
        TestContext context{ exportOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        SettingsExportCommand settingsExportCommand({});
        settingsExportCommand.Execute(context);

        auto json = ConvertToJson(exportOutput.str());
        REQUIRE(!json.isNull());
        REQUIRE_FALSE(json["adminSettings"]["LocalManifestFiles"].asBool());

        auto userSettingsFileValue = std::string(json["userSettingsFile"].asCString());
        REQUIRE(userSettingsFileValue.find("settings.json") != std::string::npos);
    }

    {
        // Enable local manifest and verify export works.
        std::ostringstream settingsOutput;
        TestContext context{ settingsOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        context.Args.AddArg(Execution::Args::Type::AdminSettingEnable, "LocalManifestFiles"sv);
        context.Override({ EnsureRunningAsAdmin, [](TestContext&) {} });
        SettingsCommand settings({});
        settings.Execute(context);

        std::ostringstream exportOutput;
        TestContext context2{ exportOutput, std::cin };
        auto previousThreadGlobals2 = context2.SetForCurrentThread();
        SettingsExportCommand settingsExportCommand({});
        settingsExportCommand.Execute(context2);
        auto json = ConvertToJson(exportOutput.str());
        REQUIRE(!json.isNull());
        REQUIRE(json["adminSettings"]["LocalManifestFiles"].asBool());

        auto userSettingsFileValue = std::string(json["userSettingsFile"].asCString());
        REQUIRE(userSettingsFileValue.find("settings.json") != std::string::npos);
    }
}
