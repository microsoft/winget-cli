// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "Commands/Common.h"
#include "AppInstallerLogging.h"
#include "Manifest/Manifest.h"
#include "AppInstallerDownloader.h"
#include "AppInstallerStrings.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/ShellExecuteInstallerHandler.h"
#include "Workflows/MsixInstallerHandler.h"

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Management::Deployment;
using namespace TestCommon;
using namespace AppInstaller::Workflow;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

class MsixInstallerHandlerTest : public MsixInstallerHandler
{
public:
    MsixInstallerHandlerTest(
        const ManifestInstaller& manifestInstaller,
        const AppInstaller::CLI::Invocation& args,
        WorkflowReporter& reporter) : MsixInstallerHandler(manifestInstaller, args, reporter) {};

protected:

    std::future<void> ExecuteInstallerAsync(const Uri& uri) override
    {
        std::filesystem::path temp = std::filesystem::temp_directory_path();
        temp /= "TestMsixInstalled.txt";
        std::ofstream file(temp, std::ofstream::out);

        file << AppInstaller::Utility::ConvertToUTF8(uri.ToString());

        file.close();

        co_return;
    }
};

class ShellExecuteInstallerHandlerTest : public ShellExecuteInstallerHandler
{
public:
    ShellExecuteInstallerHandlerTest(
        const ManifestInstaller& manifestInstaller,
        const AppInstaller::CLI::Invocation& args,
        WorkflowReporter& reporter) : ShellExecuteInstallerHandler(manifestInstaller, args, reporter) {};

    void Download() override
    {
        this->m_downloadedInstaller = TestDataFile("AppInstallerTestExeInstaller.exe");
    }

    void RenameDownloadedInstaller() override {};

    std::string TestInstallerArgs()
    {
        Download();
        return ShellExecuteInstallerHandler::GetInstallerArgs();
    }
};

class InstallFlowTest : public InstallFlow
{
public:
    InstallFlowTest(Manifest manifest, const AppInstaller::CLI::Invocation& args, std::ostream& outStream, std::istream& inStream) :
        InstallFlow(manifest, args, outStream, inStream) {}

protected:
    std::unique_ptr<InstallerHandlerBase> GetInstallerHandler() override
    {
        switch (m_selectedInstaller.InstallerType)
        {
        case ManifestInstaller::InstallerTypeEnum::Exe:
            return std::make_unique<ShellExecuteInstallerHandlerTest>(m_selectedInstaller, m_argsRef, m_reporter);
        case ManifestInstaller::InstallerTypeEnum::Msix:
            return std::make_unique<MsixInstallerHandlerTest>(m_selectedInstaller, m_argsRef, m_reporter);
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }
};

TEST_CASE("ExeInstallFlowWithTestManifest", "[InstallFlow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    auto manifest = Manifest::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yml"));

    std::ostringstream installOutput;
    AppInstaller::CLI::Invocation inv{ {""} };
    InstallFlowTest testFlow(manifest, inv, installOutput, std::cin);
    testFlow.Install();
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

TEST_CASE("InstallFlowWithNonApplicableArchitecture", "[InstallFlow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    auto manifest = Manifest::CreateFromPath(TestDataFile("InstallFlowTest_NoApplicableArchitecture.yml"));

    std::ostringstream installOutput;
    AppInstaller::CLI::Invocation inv{ {""} };
    InstallFlowTest testFlow(manifest, inv, installOutput, std::cin);
    REQUIRE_THROWS_WITH(testFlow.Install(), Catch::Contains("No installer with applicable architecture found."));
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
}

TEST_CASE("MsixInstallFlow_DownloadFlow", "[InstallFlow]")
{
    TestCommon::TempFile installResultPath("TestMsixInstalled.txt");

    // Todo: point to files from our repo when the repo goes public
    auto manifest = Manifest::CreateFromPath(TestDataFile("InstallFlowTest_Msix_DownloadFlow.yml"));

    std::ostringstream installOutput;
    AppInstaller::CLI::Invocation inv{ {""} };
    InstallFlowTest testFlow(manifest, inv, installOutput, std::cin);
    testFlow.Install();
    INFO(installOutput.str());

    // Verify Installer is called and a local file is used as package Uri.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("file://") != std::string::npos);
}

TEST_CASE("MsixInstallFlow_StreamingFlow", "[InstallFlow]")
{
    TestCommon::TempFile installResultPath("TestMsixInstalled.txt");

    // Todo: point to files from our repo when the repo goes public
    auto manifest = Manifest::CreateFromPath(TestDataFile("InstallFlowTest_Msix_StreamingFlow.yml"));

    std::ostringstream installOutput;
    AppInstaller::CLI::Invocation inv{ {""} };
    InstallFlowTest testFlow(manifest, inv, installOutput, std::cin);
    testFlow.Install();
    INFO(installOutput.str());

    // Verify Installer is called and a http address is used as package Uri.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("https://") != std::string::npos);
}

TEST_CASE("ShellExecuteHandlerInstallerArgs", "[InstallFlow]")
{
    std::ostringstream installOutput;
    WorkflowReporter reporter(installOutput, std::cin);

    {
        // Default Msi type with no args passed in, no switches specified in manifest
        auto manifest = Manifest::CreateFromPath(TestDataFile("InstallerArgTest_Msi_NoSwitches.yml"));
        AppInstaller::CLI::Invocation inv{ {""} };
        ShellExecuteInstallerHandlerTest testhandler(manifest.Installers.at(0), inv, reporter);
        std::string installerArgs = testhandler.TestInstallerArgs();
        REQUIRE(installerArgs.find("/passive") != std::string::npos);
        REQUIRE(installerArgs.find("AppInstallerTestExeInstaller.exe.log") != std::string::npos);
    }

    {
        // Msi type with /silent and /log and /custom and /installlocation, no switches specified in manifest
        auto manifest = Manifest::CreateFromPath(TestDataFile("InstallerArgTest_Msi_NoSwitches.yml"));
        AppInstaller::CLI::Invocation inv{ {""} };
        inv.AddArg(AppInstaller::CLI::ARG_SILENT);
        inv.AddArg(AppInstaller::CLI::ARG_LOG, "MyLog.log");
        inv.AddArg(AppInstaller::CLI::ARG_CUSTOM, "/customArg");
        inv.AddArg(AppInstaller::CLI::ARG_INSTALLLOCATION, "MyDir");
        ShellExecuteInstallerHandlerTest testhandler(manifest.Installers.at(0), inv, reporter);
        std::string installerArgs = testhandler.TestInstallerArgs();
        REQUIRE(installerArgs.find("/quiet") != std::string::npos);
        REQUIRE(installerArgs.find("/log \"MyLog.log\"") != std::string::npos);
        REQUIRE(installerArgs.find("/customArg") != std::string::npos);
        REQUIRE(installerArgs.find("TARGETDIR=\"MyDir\"") != std::string::npos);
    }

    {
        // Msi type with /silent and /log and /custom and /installlocation, switches specified in manifest
        auto manifest = Manifest::CreateFromPath(TestDataFile("InstallerArgTest_Msi_WithSwitches.yml"));
        AppInstaller::CLI::Invocation inv{ {""} };
        inv.AddArg(AppInstaller::CLI::ARG_SILENT);
        inv.AddArg(AppInstaller::CLI::ARG_LOG, "MyLog.log");
        inv.AddArg(AppInstaller::CLI::ARG_CUSTOM, "/customArg");
        inv.AddArg(AppInstaller::CLI::ARG_INSTALLLOCATION, "MyDir");
        ShellExecuteInstallerHandlerTest testhandler(manifest.Installers.at(0), inv, reporter);
        std::string installerArgs = testhandler.TestInstallerArgs();
        REQUIRE(installerArgs.find("/mysilent") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/mylog=\"MyLog.log\"") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/customArg") != std::string::npos); // Custom from CLI overrides manifest declaration
        REQUIRE(installerArgs.find("/myinstalldir=\"MyDir\"") != std::string::npos); // Use declaration in manifest
    }

    {
        // Default Inno type with no args passed in, no switches specified in manifest
        auto manifest = Manifest::CreateFromPath(TestDataFile("InstallerArgTest_Inno_NoSwitches.yml"));
        AppInstaller::CLI::Invocation inv{ {""} };
        ShellExecuteInstallerHandlerTest testhandler(manifest.Installers.at(0), inv, reporter);
        std::string installerArgs = testhandler.TestInstallerArgs();
        REQUIRE(installerArgs.find("/SILENT") != std::string::npos);
        REQUIRE(installerArgs.find("AppInstallerTestExeInstaller.exe.log") != std::string::npos);
    }

    {
        // Inno type with /silent and /log and /custom and /installlocation, no switches specified in manifest
        auto manifest = Manifest::CreateFromPath(TestDataFile("InstallerArgTest_Inno_NoSwitches.yml"));
        AppInstaller::CLI::Invocation inv{ {""} };
        inv.AddArg(AppInstaller::CLI::ARG_SILENT);
        inv.AddArg(AppInstaller::CLI::ARG_LOG, "MyLog.log");
        inv.AddArg(AppInstaller::CLI::ARG_CUSTOM, "/customArg");
        inv.AddArg(AppInstaller::CLI::ARG_INSTALLLOCATION, "MyDir");
        ShellExecuteInstallerHandlerTest testhandler(manifest.Installers.at(0), inv, reporter);
        std::string installerArgs = testhandler.TestInstallerArgs();
        REQUIRE(installerArgs.find("/VERYSILENT") != std::string::npos);
        REQUIRE(installerArgs.find("/LOG=\"MyLog.log\"") != std::string::npos);
        REQUIRE(installerArgs.find("/customArg") != std::string::npos);
        REQUIRE(installerArgs.find("/DIR=\"MyDir\"") != std::string::npos);
    }

    {
        // Inno type with /silent and /log and /custom and /installlocation, switches specified in manifest
        auto manifest = Manifest::CreateFromPath(TestDataFile("InstallerArgTest_Inno_WithSwitches.yml"));
        AppInstaller::CLI::Invocation inv{ {""} };
        inv.AddArg(AppInstaller::CLI::ARG_SILENT);
        inv.AddArg(AppInstaller::CLI::ARG_LOG, "MyLog.log");
        inv.AddArg(AppInstaller::CLI::ARG_CUSTOM, "/customArg");
        inv.AddArg(AppInstaller::CLI::ARG_INSTALLLOCATION, "MyDir");
        ShellExecuteInstallerHandlerTest testhandler(manifest.Installers.at(0), inv, reporter);
        std::string installerArgs = testhandler.TestInstallerArgs();
        REQUIRE(installerArgs.find("/mysilent") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/mylog=\"MyLog.log\"") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/customArg") != std::string::npos); // Custom from CLI overrides manifest declaration
        REQUIRE(installerArgs.find("/myinstalldir=\"MyDir\"") != std::string::npos); // Use declaration in manifest
    }
}