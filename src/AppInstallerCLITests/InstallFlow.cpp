// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include "TestHooks.h"
#include <AppInstallerFileLogger.h>
#include <AppInstallerStrings.h>
#include <AppInstallerSynchronization.h>
#include <Commands/InstallCommand.h>
#include <Commands/UninstallCommand.h>
#include <winget/AdminSettings.h>
#include <winget/PathVariable.h>
#include <winget/Settings.h>
#include <winget/ManifestYamlParser.h>
#include <Workflows/ArchiveFlow.h>
#include <Workflows/DownloadFlow.h>
#include <Workflows/MsiInstallFlow.h>
#include <Workflows/ShellExecuteInstallerHandler.h>

using namespace winrt::Windows::Foundation;
using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Logging;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Utility::literals;

void OverrideForDirectMsi(TestContext& context)
{
    OverrideForCheckExistingInstaller(context);

    context.Override({ DownloadInstallerFile, [](TestContext& context)
    {
        context.Add<Data::DownloadHashInfo>({ {}, {} });
        // We don't have an msi installer for tests, but we won't execute it anyway
        context.Add<Data::InstallerPath>(TestDataFile("AppInstallerTestExeInstaller.exe"));
    } });

    context.Override({ RenameDownloadedInstaller, [](TestContext&)
    {
    } });

    OverrideForUpdateInstallerMotw(context);

    context.Override({ DirectMSIInstallImpl, [](TestContext& context)
    {
            // Write out the install command
            std::filesystem::path temp = std::filesystem::temp_directory_path();
            temp /= "TestMsiInstalled.txt";
            std::ofstream file(temp, std::ofstream::out);
            file << context.Get<Execution::Data::InstallerArgs>();
            file.close();

            context.Add<Execution::Data::OperationReturnCode>(0);
        } });
}

TEST_CASE("ExeInstallFlowWithTestManifest", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/custom") != std::string::npos);
    REQUIRE(installResultStr.find("/silentwithprogress") != std::string::npos);
}

TEST_CASE("InstallFlow_RenameFromEncodedUrl", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCheckExistingInstaller(context);
    context.Override({ DownloadInstallerFile, [](TestContext& context)
    {
        context.Add<Data::DownloadHashInfo>({ {}, {} });
        auto installerPath = std::filesystem::temp_directory_path();
        installerPath /= "EncodedUrlTest.exe";
        std::filesystem::copy(TestDataFile("AppInstallerTestExeInstaller.exe"), installerPath, std::filesystem::copy_options::overwrite_existing);
        context.Add<Data::InstallerPath>(installerPath);
    } });
    OverrideForUpdateInstallerMotw(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_EncodedUrl.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/encodedUrl") != std::string::npos);
}

TEST_CASE("InstallFlow_RenameFromInvalidFileCharacterUrl", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCheckExistingInstaller(context);
    context.Override({ DownloadInstallerFile, [](TestContext& context)
    {
        context.Add<Data::DownloadHashInfo>({ {}, {} });
        auto installerPath = std::filesystem::temp_directory_path();
        installerPath /= "InvalidFileCharacterUrlTest.exe";
        std::filesystem::copy(TestDataFile("AppInstallerTestExeInstaller.exe"), installerPath, std::filesystem::copy_options::overwrite_existing);
        context.Add<Data::InstallerPath>(installerPath);
    } });
    OverrideForUpdateInstallerMotw(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_InvalidFileCharacterUrl.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/invalidFileCharacterUrl") != std::string::npos);
}

TEST_CASE("InstallFlowNonZeroExitCode", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_NonZeroExitCode.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(context.GetTerminationHR() == S_OK);
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/ExitCode 0x80070005") != std::string::npos);
    REQUIRE(installResultStr.find("/silentwithprogress") != std::string::npos);
}

TEST_CASE("InstallFlow_InstallationNotes", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_InstallationNotes.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify installation notes are displayed
    REQUIRE(context.GetTerminationHR() == S_OK);
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find("testInstallationNotes") != std::string::npos);
}

TEST_CASE("InstallFlow_UnsupportedArguments_Warn", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    TestCommon::TempDirectory tempDirectory("TempDirectory", false);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_UnsupportedArguments.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::Log, tempDirectory);

    InstallCommand install({});
    context.SetExecutingCommand(&install);
    install.Execute(context);
    INFO(installOutput.str());

    // Verify unsupported arguments warn message is shown
    REQUIRE(context.GetTerminationHR() == S_OK);
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::UnsupportedArgument).get()) != std::string::npos);
    REQUIRE(installOutput.str().find("-o,--log") != std::string::npos);
}

TEST_CASE("InstallFlow_UnsupportedArguments_Error", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    TestCommon::TempDirectory tempDirectory("TempDirectory", false);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_UnsupportedArguments.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::InstallLocation, tempDirectory);

    InstallCommand install({});
    context.SetExecutingCommand(&install);
    install.Execute(context);
    INFO(installOutput.str());

    // Verify unsupported arguments error message is shown 
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UNSUPPORTED_ARGUMENT);
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::UnsupportedArgument).get()) != std::string::npos);
    REQUIRE(installOutput.str().find("-l,--location") != std::string::npos);
}

TEST_CASE("InstallFlow_UnsupportedArguments_NotProvided")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_UnsupportedArguments.yaml").GetPath().u8string());

    InstallCommand install({});
    context.SetExecutingCommand(&install);
    install.Execute(context);
    INFO(installOutput.str());

    // Verify unsupported arguments error message is not shown when not provided
    REQUIRE(context.GetTerminationHR() == S_OK);
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::UnsupportedArgument).get()) == std::string::npos);
    REQUIRE(installOutput.str().find("-o,--log") == std::string::npos);
    REQUIRE(installOutput.str().find("-l,--location") == std::string::npos);
}

TEST_CASE("InstallFlow_ExpectedReturnCodes", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_ExpectedReturnCodes.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::Override, "/ExitCode 8"sv);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify install failed with the right message
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_INSTALL_CONTACT_SUPPORT);
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::InstallFlowReturnCodeContactSupport).get()) != std::string::npos);
    REQUIRE(installOutput.str().find("https://TestReturnResponseUrl") != std::string::npos);
}

TEST_CASE("InstallFlowWithNonApplicableArchitecture", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_NoApplicableArchitecture.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER);

    // Verify Installer was not called
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
}

TEST_CASE("InstallFlow_Zip_Exe", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    OverrideForExtractInstallerFromArchive(context);
    OverrideForVerifyAndSetNestedInstaller(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Zip_Exe.yaml").GetPath().u8string());

    TestHook::SetScanArchiveResult_Override scanArchiveResultOverride(true);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/custom") != std::string::npos);
    REQUIRE(installResultStr.find("/silentwithprogress") != std::string::npos);
}

TEST_CASE("InstallFlow_Zip_BadRelativePath", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    OverrideForExtractInstallerFromArchive(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Zip_Exe.yaml").GetPath().u8string());

    TestHook::SetScanArchiveResult_Override scanArchiveResultOverride(true);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_NESTEDINSTALLER_NOT_FOUND);

    // Verify Installer was not called
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
    auto relativePath = context.Get<Execution::Data::InstallerPath>().parent_path() / "extracted" / "relativeFilePath";
    auto expectedMessage = Resource::String::NestedInstallerNotFound(AppInstaller::Utility::LocIndString{ relativePath.u8string()});
    REQUIRE(installOutput.str().find(Resource::LocString(expectedMessage).get()) != std::string::npos);
}

TEST_CASE("InstallFlow_Zip_MissingNestedInstaller", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Zip_MissingNestedInstaller.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_INVALID_MANIFEST);

    // Verify Installer was not called
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::NestedInstallerNotSpecified).get()) != std::string::npos);
}

TEST_CASE("InstallFlow_Zip_UnsupportedNestedInstaller", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Zip_UnsupportedNestedInstaller.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE_TERMINATED_WITH(context, ERROR_NOT_SUPPORTED);

    // Verify Installer was not called
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::NestedInstallerNotSupported).get()) != std::string::npos);
}

TEST_CASE("InstallFlow_Zip_MultipleNonPortableNestedInstallers", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Zip_MultipleNonPortableNestedInstallers.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_INVALID_MANIFEST);

    // Verify Installer was not called
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::MultipleNonPortableNestedInstallersSpecified).get()) != std::string::npos);
}

TEST_CASE("InstallFlow_Zip_ArchiveScanFailed", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Zip_Exe.yaml").GetPath().u8string());

    TestHook::SetScanArchiveResult_Override scanArchiveResultOverride(false);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_ARCHIVE_SCAN_FAILED);

    // Verify Installer was not called
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::ArchiveFailedMalwareScan).get()) != std::string::npos);
}

TEST_CASE("InstallFlow_Zip_ArchiveScanOverride_AdminSettingDisabled", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Zip_Exe.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::IgnoreLocalArchiveMalwareScan);

    DisableAdminSetting(AppInstaller::Settings::BoolAdminSetting::LocalArchiveMalwareScanOverride);

    TestHook::SetScanArchiveResult_Override scanArchiveResultOverride(false);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_ARCHIVE_SCAN_FAILED);

    // Verify Installer was not called
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::ArchiveFailedMalwareScan).get()) != std::string::npos);
}

TEST_CASE("InstallFlow_Zip_ArchiveScanOverride_AdminSettingEnabled", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    OverrideForExtractInstallerFromArchive(context);
    OverrideForVerifyAndSetNestedInstaller(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Zip_Exe.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::IgnoreLocalArchiveMalwareScan);

    EnableAdminSetting(AppInstaller::Settings::BoolAdminSetting::LocalArchiveMalwareScanOverride);

    TestHook::SetScanArchiveResult_Override scanArchiveResultOverride(false);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify override message is displayed to the user.
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::ArchiveFailedMalwareScanOverridden).get()) != std::string::npos);

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/custom") != std::string::npos);
    REQUIRE(installResultStr.find("/silentwithprogress") != std::string::npos);
}

TEST_CASE("ExtractInstallerFromArchive_InvalidZip", "[InstallFlow][workflow]")
{
    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Zip_Exe.yaml"));
    context.Add<Data::Manifest>(manifest);
    context.Add<Data::Installer>(manifest.Installers.at(0));

    // Provide an invalid zip file which should be handled appropriately.
    context.Add<Data::InstallerPath>(TestDataFile("AppInstallerTestExeInstaller.exe"));
    context << ExtractFilesFromArchive;
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_EXTRACT_ARCHIVE_FAILED);
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::ExtractArchiveFailed).get()) != std::string::npos);
}

TEST_CASE("ExtractInstallerFromArchiveWithTar", "[InstallFlow][workflow]")
{
    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::ArchiveExtractionMethod>(AppInstaller::Archive::ExtractionMethod::Tar);

    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();

    OverrideForShellExecute(context);
    OverrideForVerifyAndSetNestedInstaller(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Zip_Exe.yaml").GetPath().u8string());

    TestHook::SetScanArchiveResult_Override scanArchiveResultOverride(true);
    TestHook::SetExtractArchiveWithTarResult_Override setExtractArchiveWithTarResultOverride(ERROR_SUCCESS);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::ExtractArchiveSucceeded).get()) != std::string::npos);

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/custom") != std::string::npos);
    REQUIRE(installResultStr.find("/silentwithprogress") != std::string::npos);
}

TEST_CASE("ExtractInstallerFromArchiveWithTar_InvalidZip", "[InstallFlow][workflow]")
{
    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::ArchiveExtractionMethod>(AppInstaller::Archive::ExtractionMethod::Tar);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Zip_Exe.yaml"));
    context.Add<Data::Manifest>(manifest);
    context.Add<Data::Installer>(manifest.Installers.at(0));

    // Provide an invalid zip file which should be handled appropriately.
    context.Add<Data::InstallerPath>(TestDataFile("AppInstallerTestExeInstaller.exe"));
    context << ExtractFilesFromArchive;
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_EXTRACT_ARCHIVE_FAILED);
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::ExtractArchiveFailed).get()) != std::string::npos);
}

TEST_CASE("MSStoreInstallFlowWithTestManifest", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestMSStoreInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForMSStore(context, false);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_MSStore.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("9WZDNCRFJ364") != std::string::npos);
}

TEST_CASE("MsixInstallFlow_DownloadFlow", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestMsixInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForMSIX(context);
    OverrideForUpdateInstallerMotw(context);
    // Todo: point to files from our repo when the repo goes public
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Msix_DownloadFlow.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and a local file is used as package Uri.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    Uri uri = Uri(ConvertToUTF16(installResultStr));
    REQUIRE(uri.SchemeName() == L"file");
}

TEST_CASE("MsixInstallFlow_StreamingFlow", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestMsixInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForMSIX(context);
    OverrideForCheckExistingInstaller(context);
    // Todo: point to files from our repo when the repo goes public
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Msix_StreamingFlow.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and a http address is used as package Uri.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    Uri uri = Uri(ConvertToUTF16(installResultStr));
    REQUIRE(uri.SchemeName() == L"https");
}

TEST_CASE("MsiInstallFlow_DirectMsi", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestMsiInstalled.txt");

    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::EFDirectMSI>(true);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForDirectMsi(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallerArgTest_Msi_NoSwitches.yaml").GetPath().u8string());
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
    REQUIRE(installResultStr.find("/quiet") != std::string::npos);
}

TEST_CASE("InstallFlow_Portable", "[InstallFlow][workflow]")
{
    TestCommon::TempDirectory tempDirectory("TestPortableInstallRoot", false);
    TestCommon::TempFile portableInstallResultPath("TestPortableInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForPortableInstallFlow(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Portable.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::InstallLocation, tempDirectory);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE(std::filesystem::exists(portableInstallResultPath.GetPath()));
}

TEST_CASE("InstallFlow_Portable_SymlinkCreationFail", "[InstallFlow][workflow]")
{
    TestCommon::TempDirectory tempDirectory("TestPortableInstallRoot", false);
    std::ostringstream installOutput;
    TestContext installContext{ installOutput, std::cin };
    auto PreviousThreadGlobals = installContext.SetForCurrentThread();
    OverridePortableInstaller(installContext);
    TestHook::SetCreateSymlinkResult_Override createSymlinkResultOverride(false);
    const auto& targetDirectory = tempDirectory.GetPath();
    const auto& portableTargetPath = targetDirectory / "AppInstallerTestExeInstaller.exe";
    installContext.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Portable.yaml").GetPath().u8string());
    installContext.Args.AddArg(Execution::Args::Type::InstallLocation, targetDirectory.u8string());
    installContext.Args.AddArg(Execution::Args::Type::InstallScope, "user"sv);

    InstallCommand install({});
    install.Execute(installContext);

    {
        INFO(installOutput.str());

        // Use CHECK to allow the uninstall to still occur
        CHECK(std::filesystem::exists(portableTargetPath));
        CHECK(AppInstaller::Registry::Environment::PathVariable(AppInstaller::Manifest::ScopeEnum::User).Contains(targetDirectory));
    }

    // Perform uninstall
    std::ostringstream uninstallOutput;
    TestContext uninstallContext{ uninstallOutput, std::cin };
    auto previousThreadGlobals = uninstallContext.SetForCurrentThread();
    uninstallContext.Args.AddArg(Execution::Args::Type::Name, "AppInstaller Test Portable Exe"sv);
    uninstallContext.Args.AddArg(Execution::Args::Type::AcceptSourceAgreements);

    UninstallCommand uninstall({});
    uninstall.Execute(uninstallContext);
    INFO(uninstallOutput.str());
    REQUIRE_FALSE(std::filesystem::exists(portableTargetPath));
}

TEST_CASE("PortableInstallFlow_UserScope", "[InstallFlow][workflow]")
{
    TestCommon::TempDirectory tempDirectory("TestPortableInstallRoot", false);
    TestCommon::TempFile portableInstallResultPath("TestPortableInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForPortableInstallFlow(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Portable.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::InstallLocation, tempDirectory);
    context.Args.AddArg(Execution::Args::Type::InstallScope, "user"sv);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE(std::filesystem::exists(portableInstallResultPath.GetPath()));
}

TEST_CASE("PortableInstallFlow_MachineScope", "[InstallFlow][workflow]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    TestCommon::TempDirectory tempDirectory("TestPortableInstallRoot", false);
    TestCommon::TempFile portableInstallResultPath("TestPortableInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForPortableInstallFlow(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Portable.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::InstallLocation, tempDirectory);
    context.Args.AddArg(Execution::Args::Type::InstallScope, "machine"sv);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());
    REQUIRE(std::filesystem::exists(portableInstallResultPath.GetPath()));
}

TEST_CASE("ShellExecuteHandlerInstallerArgs", "[InstallFlow][workflow]")
{
    {
        std::ostringstream installOutput;
        TestContext context{ installOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        // Default Msi type with no args passed in, no switches specified in manifest
        auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallerArgTest_Msi_NoSwitches.yaml"));
        context.Add<Data::Manifest>(manifest);
        context.Add<Data::Installer>(manifest.Installers.at(0));
        context.Add<Data::InstallerPath>(TestDataFile("AppInstallerTestExeInstaller.exe"));
        context << GetInstallerArgs;
        std::string installerArgs = context.Get<Data::InstallerArgs>();
        REQUIRE(installerArgs.find("/passive") != std::string::npos);
        REQUIRE(installerArgs.find(FileLogger::DefaultPrefix()) != std::string::npos);
        REQUIRE(installerArgs.find(manifest.Id) != std::string::npos);
        REQUIRE(installerArgs.find(manifest.Version) != std::string::npos);
    }

    {
        std::ostringstream installOutput;
        TestContext context{ installOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        // Msi type with /silent and /log and /custom and /installlocation, no switches specified in manifest
        auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallerArgTest_Msi_NoSwitches.yaml"));
        context.Args.AddArg(Execution::Args::Type::Silent);
        context.Args.AddArg(Execution::Args::Type::Log, "MyLog.log"sv);
        context.Args.AddArg(Execution::Args::Type::InstallLocation, "MyDir"sv);
        context.Add<Data::Manifest>(manifest);
        context.Add<Data::Installer>(manifest.Installers.at(0));
        context << GetInstallerArgs;
        std::string installerArgs = context.Get<Data::InstallerArgs>();
        REQUIRE(installerArgs.find("/quiet") != std::string::npos);
        REQUIRE(installerArgs.find("/log \"MyLog.log\"") != std::string::npos);
        REQUIRE(installerArgs.find("TARGETDIR=\"MyDir\"") != std::string::npos);
    }

    {
        std::ostringstream installOutput;
        TestContext context{ installOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        // Msi type with /silent and /log and /custom and /installlocation, switches specified in manifest
        auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallerArgTest_Msi_WithSwitches.yaml"));
        context.Args.AddArg(Execution::Args::Type::Silent);
        context.Args.AddArg(Execution::Args::Type::Log, "MyLog.log"sv);
        context.Args.AddArg(Execution::Args::Type::InstallLocation, "MyDir"sv);
        context.Add<Data::Manifest>(manifest);
        context.Add<Data::Installer>(manifest.Installers.at(0));
        context << GetInstallerArgs;
        std::string installerArgs = context.Get<Data::InstallerArgs>();
        REQUIRE(installerArgs.find("/mysilent") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/mylog=\"MyLog.log\"") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/mycustom") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/myinstalldir=\"MyDir\"") != std::string::npos); // Use declaration in manifest
    }

    {
        std::ostringstream installOutput;
        TestContext context{ installOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        // Default Inno type with no args passed in, no switches specified in manifest
        auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallerArgTest_Inno_NoSwitches.yaml"));
        context.Add<Data::Manifest>(manifest);
        context.Add<Data::Installer>(manifest.Installers.at(0));
        context.Add<Data::InstallerPath>(TestDataFile("AppInstallerTestExeInstaller.exe"));
        context << GetInstallerArgs;
        std::string installerArgs = context.Get<Data::InstallerArgs>();
        REQUIRE(installerArgs.find("/SILENT") != std::string::npos);
        REQUIRE(installerArgs.find(FileLogger::DefaultPrefix()) != std::string::npos);
        REQUIRE(installerArgs.find(manifest.Id) != std::string::npos);
        REQUIRE(installerArgs.find(manifest.Version) != std::string::npos);
    }

    {
        std::ostringstream installOutput;
        TestContext context{ installOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        // Inno type with /silent and /log and /custom and /installlocation, no switches specified in manifest
        auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallerArgTest_Inno_NoSwitches.yaml"));
        context.Args.AddArg(Execution::Args::Type::Silent);
        context.Args.AddArg(Execution::Args::Type::Log, "MyLog.log"sv);
        context.Args.AddArg(Execution::Args::Type::InstallLocation, "MyDir"sv);
        context.Add<Data::Manifest>(manifest);
        context.Add<Data::Installer>(manifest.Installers.at(0));
        context << GetInstallerArgs;
        std::string installerArgs = context.Get<Data::InstallerArgs>();
        REQUIRE(installerArgs.find("/VERYSILENT") != std::string::npos);
        REQUIRE(installerArgs.find("/LOG=\"MyLog.log\"") != std::string::npos);
        REQUIRE(installerArgs.find("/DIR=\"MyDir\"") != std::string::npos);
    }

    {
        std::ostringstream installOutput;
        TestContext context{ installOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        // Inno type with /silent and /log and /custom and /installlocation, switches specified in manifest
        auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallerArgTest_Inno_WithSwitches.yaml"));
        context.Args.AddArg(Execution::Args::Type::Silent);
        context.Args.AddArg(Execution::Args::Type::Log, "MyLog.log"sv);
        context.Args.AddArg(Execution::Args::Type::InstallLocation, "MyDir"sv);
        context.Add<Data::Manifest>(manifest);
        context.Add<Data::Installer>(manifest.Installers.at(0));
        context << GetInstallerArgs;
        std::string installerArgs = context.Get<Data::InstallerArgs>();
        REQUIRE(installerArgs.find("/mysilent") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/mylog=\"MyLog.log\"") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/mycustom") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/myinstalldir=\"MyDir\"") != std::string::npos); // Use declaration in manifest
    }

    {
        std::ostringstream installOutput;
        TestContext context{ installOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        // Inno type with /silent and /log and /custom and /installlocation, switches specified in manifest and --custom argument used in cli
        auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallerArgTest_Inno_WithSwitches.yaml"));
        context.Args.AddArg(Execution::Args::Type::Silent);
        context.Args.AddArg(Execution::Args::Type::Log, "MyLog.log"sv);
        context.Args.AddArg(Execution::Args::Type::InstallLocation, "MyDir"sv);
        context.Args.AddArg(Execution::Args::Type::CustomSwitches, "/MyAppendedSwitch"sv);
        context.Add<Data::Manifest>(manifest);
        context.Add<Data::Installer>(manifest.Installers.at(0));
        context << GetInstallerArgs;
        std::string installerArgs = context.Get<Data::InstallerArgs>();
        REQUIRE(installerArgs.find("/mysilent") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/mylog=\"MyLog.log\"") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/mycustom") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/myinstalldir=\"MyDir\"") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/MyAppendedSwitch") != std::string::npos); // Use declaration from argument
    }

    {
        std::ostringstream installOutput;
        TestContext context{ installOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        // Inno type with /silent and /log and /custom and /installlocation, switches specified in manifest and whitespace-only --custom argument used in cli
        auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallerArgTest_Inno_WithSwitches.yaml"));
        context.Args.AddArg(Execution::Args::Type::Silent);
        context.Args.AddArg(Execution::Args::Type::CustomSwitches, "\t"sv);
        context.Add<Data::Manifest>(manifest);
        context.Add<Data::Installer>(manifest.Installers.at(0));
        context << GetInstallerArgs;
        std::string installerArgs = context.Get<Data::InstallerArgs>();
        REQUIRE(installerArgs.find("/mysilent") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("\t") == std::string::npos); // Whitespace only Custom switches should not be appended
    }

    {
        std::ostringstream installOutput;
        TestContext context{ installOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        // Override switch specified. The whole arg passed to installer is overridden.
        auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallerArgTest_Inno_WithSwitches.yaml"));
        context.Args.AddArg(Execution::Args::Type::Silent);
        context.Args.AddArg(Execution::Args::Type::Log, "MyLog.log"sv);
        context.Args.AddArg(Execution::Args::Type::InstallLocation, "MyDir"sv);
        context.Args.AddArg(Execution::Args::Type::Override, "/OverrideEverything"sv);
        context.Add<Data::Manifest>(manifest);
        context.Add<Data::Installer>(manifest.Installers.at(0));
        context << GetInstallerArgs;
        std::string installerArgs = context.Get<Data::InstallerArgs>();
        REQUIRE(installerArgs == "/OverrideEverything"); // Use value specified in override switch
    }
}

TEST_CASE("InstallFlow_SearchAndInstall", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForOpenSource(context, CreateTestSource({ TSR::TestQuery_ReturnOne }), true);
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestQuery_ReturnOne.Query);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/custom") != std::string::npos);
    REQUIRE(installResultStr.find("/silentwithprogress") != std::string::npos);
}

TEST_CASE("InstallFlow_SearchFoundNoApp", "[InstallFlow][workflow]")
{
    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForOpenSource(context, CreateTestSource({}), true);
    context.Args.AddArg(Execution::Args::Type::Query, "TestQueryReturnZero"sv);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify proper message is printed
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::NoPackageFound).get()) != std::string::npos);
}

TEST_CASE("InstallFlow_SearchFoundMultipleApp", "[InstallFlow][workflow]")
{
    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForOpenSource(context, CreateTestSource({ TSR::TestQuery_ReturnTwo }), true);
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestQuery_ReturnTwo.Query);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify proper message is printed
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::MultiplePackagesFound).get()) != std::string::npos);
}

TEST_CASE("InstallFlow_LicenseAgreement", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_LicenseAgreement.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::AcceptPackageAgreements);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify agreements are shown
    REQUIRE(installOutput.str().find("Agreement with text") != std::string::npos);
    REQUIRE(installOutput.str().find("This is the text of the agreement.") != std::string::npos);
    REQUIRE(installOutput.str().find("Agreement with URL") != std::string::npos);
    REQUIRE(installOutput.str().find("https://TestAgreementUrl") != std::string::npos);

    // Verify Installer is called.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
}

TEST_CASE("InstallFlow_LicenseAgreement_Prompt", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    // Accept the agreements by saying "Yes" at the prompt
    std::istringstream installInput{ "y" };

    std::ostringstream installOutput;
    TestContext context{ installOutput, installInput };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_LicenseAgreement.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify prompt was shown
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::PackageAgreementsPrompt).get()) != std::string::npos);

    // Verify agreements are shown
    REQUIRE(installOutput.str().find("Agreement with text") != std::string::npos);
    REQUIRE(installOutput.str().find("This is the text of the agreement.") != std::string::npos);
    REQUIRE(installOutput.str().find("Agreement with URL") != std::string::npos);
    REQUIRE(installOutput.str().find("https://TestAgreementUrl") != std::string::npos);

    // Verify Installer is called.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
}

TEST_CASE("InstallFlow_LicenseAgreement_NotAccepted", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    // Say "No" at the agreements prompt
    std::istringstream installInput{ "n" };

    std::ostringstream installOutput;
    TestContext context{ installOutput, installInput };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_LicenseAgreement.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify agreements are shown
    REQUIRE(installOutput.str().find("Agreement with text") != std::string::npos);
    REQUIRE(installOutput.str().find("This is the text of the agreement.") != std::string::npos);
    REQUIRE(installOutput.str().find("Agreement with URL") != std::string::npos);
    REQUIRE(installOutput.str().find("https://TestAgreementUrl") != std::string::npos);

    // Verify installation failed
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_PACKAGE_AGREEMENTS_NOT_ACCEPTED);
    REQUIRE_FALSE(std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::PackageAgreementsNotAgreedTo).get()) != std::string::npos);
}

TEST_CASE("InstallFlowMultiLocale_RequirementNotSatisfied", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("Manifest-Good-MultiLocale.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::Locale, "en-US"sv);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER);

    // Verify Installer was not called
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
}

TEST_CASE("InstallFlowMultiLocale_RequirementSatisfied", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("Manifest-Good-MultiLocale.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::Locale, "fr-FR"sv);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/fr-FR") != std::string::npos);
}

TEST_CASE("InstallFlowMultiLocale_PreferenceNoBetterLocale", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("Manifest-Good-MultiLocale.yaml").GetPath().u8string());

    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::InstallLocalePreference>({ "zh-CN" });

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/unknown") != std::string::npos);
}

TEST_CASE("InstallFlowMultiLocale_PreferenceWithBetterLocale", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("Manifest-Good-MultiLocale.yaml").GetPath().u8string());

    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::InstallLocalePreference>({ "en-US" });

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/en-GB") != std::string::npos);
}

TEST_CASE("InstallFlow_InstallMultiple", "[InstallFlow][workflow][MultiQuery]")
{
    TestCommon::TempFile exeInstallResultPath("TestExeInstalled.txt");
    TestCommon::TempFile msixInstallResultPath("TestMsixInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForMSIX(context);
    OverrideForShellExecute(context);
    OverrideForOpenSource(context, CreateTestSource({ TSR::TestInstaller_Exe, TSR::TestInstaller_Msix }), true);
    context.Args.AddArg(Execution::Args::Type::MultiQuery, TSR::TestInstaller_Exe.Query);
    context.Args.AddArg(Execution::Args::Type::MultiQuery, TSR::TestInstaller_Msix.Query);

    InstallCommand installCommand({});
    installCommand.Execute(context);
    INFO(installOutput.str());

    // Verify all packages were installed
    REQUIRE(std::filesystem::exists(exeInstallResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(msixInstallResultPath.GetPath()));
}

TEST_CASE("InstallFlow_InstallMultiple_SearchFailed", "[InstallFlow][workflow][MultiQuery]")
{
    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForOpenSource(context, CreateTestSource({ TSR::TestInstaller_Exe }), true);
    context.Args.AddArg(Execution::Args::Type::MultiQuery, TSR::TestInstaller_Exe.Query);
    context.Args.AddArg(Execution::Args::Type::MultiQuery, TSR::TestInstaller_Msix.Query);

    InstallCommand installCommand({});
    installCommand.Execute(context);
    INFO(installOutput.str());

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_NOT_ALL_QUERIES_FOUND_SINGLE);
}

TEST_CASE("InstallFlow_InstallAcquiresLock", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForOpenSource(context, CreateTestSource({ TSR::TestQuery_ReturnOne }), true);
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Query, TSR::TestQuery_ReturnOne.Query);

    wil::unique_event enteredShellExecute;
    enteredShellExecute.create();
    wil::unique_event canLeaveShellExecute;
    canLeaveShellExecute.create();
    AppInstaller::ProgressCallback progress;

    context.Override({ ShellExecuteInstallImpl, [&](TestContext& context)
        {
            enteredShellExecute.SetEvent();
            canLeaveShellExecute.wait(500);
            ShellExecuteInstallImpl(context);
        }});

    {
        std::thread otherThread([&]() {
            InstallCommand install({});
            install.Execute(context);
            });

        REQUIRE(enteredShellExecute.wait(5000));

        AppInstaller::Synchronization::CrossProcessInstallLock mainThreadLock;
        REQUIRE(!mainThreadLock.TryAcquireNoWait());

        canLeaveShellExecute.SetEvent();
        otherThread.join();
    }

    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/custom") != std::string::npos);
    REQUIRE(installResultStr.find("/silentwithprogress") != std::string::npos);
}

TEST_CASE("InstallFlow_InstallWithReboot", "[InstallFlow][workflow][reboot]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    TestCommon::TestUserSettings testSettings;

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);

    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_ExpectedReturnCodes.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::AllowReboot);

    context.Override({ ShellExecuteInstallImpl, [&](TestContext& context)
    {
        // APPINSTALLER_CLI_ERROR_INSTALL_REBOOT_REQUIRED_TO_INSTALL (should be treated as an installer error)
        context.Add<Data::OperationReturnCode>(10);
    } });

    SECTION("Reboot success")
    {
        TestHook::SetInitiateRebootResult_Override initiateRebootResultOverride(true);

        InstallCommand install({});
        install.Execute(context);
        INFO(installOutput.str());

        REQUIRE(context.IsTerminated());
        REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
        REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::InitiatingReboot).get()) != std::string::npos);
        REQUIRE_FALSE(installOutput.str().find(Resource::LocString(Resource::String::FailedToInitiateReboot).get()) != std::string::npos);
    }
    SECTION("Reboot failed")
    {
        TestHook::SetInitiateRebootResult_Override initiateRebootResultOverride(false);

        InstallCommand install({});
        install.Execute(context);
        INFO(installOutput.str());

        REQUIRE(context.IsTerminated());
        REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
        REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::InitiatingReboot).get()) != std::string::npos);
        REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::FailedToInitiateReboot).get()) != std::string::npos);
    }
}
